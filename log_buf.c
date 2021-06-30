/**
*
*  \file        log_buf.c
*  \brief       logbuf core implementation.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/

#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#ifdef LINUX
#include <sys/time.h>
#endif
#define LOGBUF_VA
#include "log_synch.h"
#include "logbuf.h"
#include "logdest.h"
#include "logcounter.h"

/*
 * TODO:
 *  - remove special types for TIME, FMT, use I64 & STR instead
 *  - add support for RID
 *  - use 255 arg for primary fmtstr & timestamp
 */

/* 
 * Buffer data format:
 * u8 MID_TYPE_ID
 * u32 MSGID
 * u8 GRP_TYPE_ID
 * u32 GRPID
 * u8 arg0_TYPE_ID
 * u8 arg0_ARG_NUM
 * ...
 */


void (*logbuf_assert_hook)(const char *fmt, ...) = NULL;

struct _lh_list {
 struct _lh_list *lh_next;
 struct _logdest *lh_dest;
 uint32_t lh_lost;
 uint64_t lh_mask;
 int lh_tstamp;
};

#define LOGBUF_INIT_BUF 256
  
struct _logbuf {
 uint8_t *lb_xbuf;
 uint8_t *lb_pos;
 uint32_t lb_left;
 uint8_t lb_buf[LOGBUF_INIT_BUF];
 uint32_t lb_ev_type;
 logcounter_t *lb_lc;
 int  lb_ok;
};

struct _logcounter {
 struct _lh_list *lc_handles;
 uint64_t lc_ev_default_mask;
 uint64_t lc_ev_mask;
 logbuf_mutex_t lc_lock;
 logbuf_cvar_t  lc_list_cv;
 uint32_t lc_list_writers;
 uint32_t lc_list_readers;
 logbuf_mutex_t lc_lost_cnt_lock;
 int lc_tstamp_on;
};

static void incr_lost(logcounter_t *lc);

struct _logcounter*
logcounter_create(void)
{
 logcounter_t *lc;
 
 lc = malloc(sizeof(*lc));
 if(lc == NULL) return NULL;
 memset(lc, 0, sizeof(*lc));
 logbuf_mutex_init(&lc->lc_lock);
 logbuf_mutex_init(&lc->lc_lost_cnt_lock);
 logbuf_cvar_init(&lc->lc_list_cv);
 return lc;
}

void 
logcounter_destroy(logcounter_t *lc)
{
 while(lc->lc_handles != NULL) 
  logcounter_disconnect(lc, lc->lc_handles->lh_dest);

 logbuf_mutex_destroy(&lc->lc_lock);
 logbuf_cvar_destroy(&lc->lc_list_cv);
 logbuf_mutex_destroy(&lc->lc_lost_cnt_lock);
 free(lc);
}

logbuf_t* 
logbuf_create(uint64_t ev_type, uint32_t msg_id, int add_timestamp)
{
 logbuf_t *buf;
 uint8_t *pos;

 buf = malloc(sizeof(*buf));
 if(!buf) return 0;
 buf->lb_ev_type = ev_type;
 buf->lb_ok = 1;
 buf->lb_xbuf = NULL;
 pos = buf->lb_buf;
 *pos = LOGBUF_T_MID; pos++;
 logbuf_put32(msg_id, pos); pos += 4;
 *pos = LOGBUF_T_GRP; pos++;
 memcpy(pos, &ev_type, 8); pos += 8;
 if(add_timestamp) {
  uint64_t time;
#if defined(WIN32)
  LARGE_INTEGER freq;
  LARGE_INTEGER pcount;
  uint64_t time_s, time_n;

  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&pcount);
  time_n = pcount.QuadPart;
  time_s = time_n/freq.QuadPart;
  time_n -= time_s*freq.QuadPart;
  /* max value for time_n is 18446744073 which is max frequency = 18GHz */
  time_n = time_n*1000000000/freq.QuadPart; 
  time = time_s*1000000000 + time_n;
#elif defined(LINUX)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time = (uint64_t)tv.tv_sec*1000000000 + (uint64_t)tv.tv_usec*1000;
#else
  time = 0;
#endif
  *pos = LOGBUF_T_TIME; pos++;
  *pos = 0; pos++; /* arg num 0 */
  memcpy(pos, &time, 8); pos+= 8;
 }
 buf->lb_pos = pos;
 buf->lb_left = sizeof(buf->lb_buf) - (pos - buf->lb_buf);
 return buf;
}

const uint8_t*
logbuf_raw(logbuf_t *b, uint32_t *len)
{
 uint8_t *buf;

 buf = b->lb_xbuf ? b->lb_xbuf : b->lb_buf;
 *len = b->lb_pos - buf;
 return buf;
}

int 
logbuf_event_needed(struct _logcounter *lc, uint64_t ev_type)
{
 return (lc->lc_ev_mask & ev_type) != 0;
}

logbuf_t* 
logbuf_get(struct _logcounter *lc, uint64_t ev_type, uint32_t msg_id)
{
 logbuf_t *buf;
 if((lc->lc_ev_mask & ev_type) == 0) return NULL;
 buf = logbuf_create(ev_type, msg_id, lc->lc_tstamp_on);
 if(!buf) {
  logbuf_mutex_enter(&lc->lc_lock);
  incr_lost(lc);
  logbuf_mutex_exit(&lc->lc_lock);
  return NULL;
 }
 buf->lb_lc = lc;
 return buf;
}

logbuf_t*
logbuf_regenerate(uint8_t *src_buf, uint32_t len)
{
 uint64_t ev_type;
 logbuf_t *buf;
 uint8_t *b;

 if(len < 1 + 4 + 1 + 8) return NULL; /* not using logdest_get_arg
                                         to avoid dependency */
 if(src_buf[1+4] != LOGBUF_T_GRP || src_buf[0] != LOGBUF_T_MID) return NULL;
 memcpy(&ev_type, src_buf+1+4+1, 8);
 buf = malloc(sizeof(*buf));
 if(!buf) return NULL;
 if(len > sizeof(buf->lb_buf)) {
  b = malloc(len);
  if(b == NULL) {
   free(buf);
   return NULL;
  }
  buf->lb_xbuf = b;
  buf->lb_left = 0;
 } else {
  buf->lb_xbuf = NULL;
  b = buf->lb_buf;
  buf->lb_left = sizeof(buf->lb_buf) - len;
 }
 memcpy(b, src_buf, len);
 buf->lb_pos = b+len;
 buf->lb_ev_type = ev_type;
 buf->lb_ok = 1;
 return buf;
}

void
logbuf_assign_counter(logbuf_t *lb, logcounter_t *lc)
{
 lb->lb_lc = lc;
}

void
logbuf_free(logbuf_t *b)
{
 if(b->lb_xbuf) free(b->lb_xbuf);
 free(b);
}

static void 
incr_lost(struct _logcounter *lc)
{
 struct _lh_list *h;
 logbuf_mutex_enter(&lc->lc_lost_cnt_lock);
 for(h = lc->lc_handles; h; h = h->lh_next) h->lh_lost++;
 logbuf_mutex_exit(&lc->lc_lost_cnt_lock);
}

static int
send_lost(logcounter_t *lc, struct _logdest *ld, uint32_t count)
{
 uint8_t lost_msg[10];
 uint8_t *pos = lost_msg;
 *pos = LOGBUF_T_MID; pos++;
 logbuf_put32(LOGBUF_ID_LOST, pos); pos+=4;
 *pos = LOGBUF_T_I32; pos++;
 logbuf_put32(count, pos);
 return ld->ld_send(ld, lost_msg, 10);
}

void 
logbuf_send(logbuf_t *b)
{
 uint32_t sz;
 uint32_t ev_type;
 struct _lh_list *h;
 logcounter_t *lc;
 uint8_t *buf;

 lc = b->lb_lc;
 if(!b->lb_ok) {
  logbuf_mutex_enter(&lc->lc_lock);
  incr_lost(lc);
  logbuf_mutex_exit(&lc->lc_lock);
  logbuf_free(b);
  return;
 }

 ev_type = b->lb_ev_type;
 lc = b->lb_lc;
 buf = b->lb_xbuf ? b->lb_xbuf : b->lb_buf;
 sz = b->lb_pos - buf;
 logbuf_mutex_enter(&lc->lc_lock);
 while(lc->lc_list_writers != 0) logbuf_cvar_wait(&lc->lc_list_cv, &lc->lc_lock);
 lc->lc_list_readers++;
 logbuf_mutex_exit(&lc->lc_lock);
 for(h = lc->lc_handles; h; h = h->lh_next) {
  uint32_t lost;
  if((ev_type & h->lh_mask) == 0) continue;
  logbuf_mutex_enter(&lc->lc_lost_cnt_lock);
  lost = h->lh_lost;
  h->lh_lost = 0;
  logbuf_mutex_exit(&lc->lc_lost_cnt_lock);
  if(lost) {
    if(!send_lost(lc, h->lh_dest, lost)) {
     logbuf_mutex_enter(&lc->lc_lost_cnt_lock);
	 h->lh_lost += lost + 1;
     logbuf_mutex_exit(&lc->lc_lost_cnt_lock);
	 continue;
	}
  }
  if(!h->lh_dest->ld_send(h->lh_dest, buf, sz)) {
   logbuf_mutex_enter(&lc->lc_lost_cnt_lock);
   h->lh_lost++;
   logbuf_mutex_exit(&lc->lc_lost_cnt_lock);
  }
 }
 logbuf_free(b);
 logbuf_mutex_enter(&lc->lc_lock);
 if(--lc->lc_list_readers == 0 && lc->lc_list_writers != 0) logbuf_cvar_broadcast(&lc->lc_list_cv);
 logbuf_mutex_exit(&lc->lc_lock);
}

static void
recalc_mask(logcounter_t *lc)
{
 uint32_t new_mask = 0;
 struct _lh_list *e;
 for(e = lc->lc_handles; e != NULL; e = e->lh_next) new_mask |= e->lh_mask;
 lc->lc_ev_mask = new_mask;
}

void 
logcounter_disconnect(logcounter_t *lc, logdest_t *h)
{
 struct _lh_list **e, *e0;
 logbuf_mutex_enter(&lc->lc_lock);
 lc->lc_list_writers++;
 while(lc->lc_list_readers != 0) logbuf_cvar_wait(&lc->lc_list_cv, &lc->lc_lock);
 for(e = &lc->lc_handles; *e && (*e)->lh_dest != h; e = &(*e)->lh_next);
 e0 = *e;
 if(e0) {
  *e = e0->lh_next;
  lc->lc_tstamp_on -= e0->lh_tstamp;
  recalc_mask(lc);
  free(e0);
 }
 if(!--lc->lc_list_writers) logbuf_cvar_broadcast(&lc->lc_list_cv);
 logbuf_mutex_exit(&lc->lc_lock);
}

static struct _lh_list* find_log_dest(logcounter_t *lc, logdest_t *h)
{
 struct _lh_list *e;
 for(e = lc->lc_handles; e && e->lh_dest != h; e = e->lh_next);
 return e;
}

void
logcounter_set_filter(logcounter_t *lc, logdest_t *h, uint64_t f)
{
 struct _lh_list *e;
 logbuf_mutex_enter(&lc->lc_lock);
 e = find_log_dest(lc, h);
 if(e == NULL) logbuf_assert_hook("logcounter_set_filter: logdest is NULL");
 e->lh_mask = f;
 if((lc->lc_ev_mask & f) != lc->lc_ev_mask) recalc_mask(lc);
 else lc->lc_ev_mask |= f;
 logbuf_mutex_exit(&lc->lc_lock);
 return;
}

void
logcounter_set_default_filter(logcounter_t *lc, uint64_t f)
{
 logbuf_mutex_enter(&lc->lc_lock);
 lc->lc_ev_default_mask = f;
 logbuf_mutex_exit(&lc->lc_lock);
}

void
logcounter_tstamp_onoff(logcounter_t *lc, logdest_t *h, int enable)
{
 struct _lh_list *e;
 logbuf_mutex_enter(&lc->lc_lock);
 e = find_log_dest(lc, h);
 if(e == NULL) logbuf_assert_hook("logcounter_tstamp_onoff: logdest is NULL");
 enable = !!enable;
 if(e->lh_tstamp != enable) {
  if(enable) lc->lc_tstamp_on++;
  else lc->lc_tstamp_on--;
  e->lh_tstamp = enable;
 }
 logbuf_mutex_exit(&lc->lc_lock);
 return;
}

int
logcounter_connect(logcounter_t *lc, logdest_t *h)
{
 struct _lh_list *e;

 e = malloc(sizeof(*e));
 if(!e) return 0;

 e->lh_dest = h;
 e->lh_lost = 0;
 e->lh_mask = lc->lc_ev_default_mask;
 e->lh_tstamp = 0;
 logbuf_mutex_enter(&lc->lc_lock);
 if(find_log_dest(lc, h) != NULL) logbuf_assert_hook("logcounter_connect: logdest %p is already connected", h);
 e->lh_next = lc->lc_handles;
 lc->lc_handles = e;
 lc->lc_ev_mask |= lc->lc_ev_default_mask;
 logbuf_mutex_exit(&lc->lc_lock);
 return 1;
}

static void
log_buf_grow(logbuf_t *b, uint32_t ammt)
{
 if(!b->lb_xbuf) {
  uint32_t buf_used = sizeof(b->lb_buf) - b->lb_left;
  /* initialize the dynamic buffer */
  b->lb_xbuf = malloc(ammt+buf_used);
  if(!b->lb_xbuf) {
   b->lb_ok = 0;
   return;
  }
  memcpy(b->lb_xbuf, b->lb_buf, buf_used);
  b->lb_pos = b->lb_xbuf+buf_used;
  b->lb_left = ammt;
 } else {
  uint8_t *nb;
  uint32_t used =  b->lb_pos-b->lb_xbuf;
  uint32_t oldsz = used + b->lb_left;
  uint32_t newsz = oldsz<<1;

  if(newsz < ammt+used) newsz = ammt+used;
  nb = malloc(newsz);
  if(!nb) {
   b->lb_ok = 0;
   return;
  }
  memcpy(nb, b->lb_xbuf, used);
  free(b->lb_xbuf);
  b->lb_xbuf = nb;
  b->lb_pos = nb + used;
  b->lb_left = newsz - used;
 }
}

static void
log_buf_write(logbuf_t *b, const uint8_t *p, uint32_t n)
{
 if(b->lb_left < n) {
  log_buf_grow(b, n);
  if(!b->lb_ok) return;
 }
 memcpy(b->lb_pos, p, n);
 b->lb_pos += n;
 b->lb_left -= n;
}

static void
log_buf_put_arg(logbuf_t *b, uint32_t argn)
{
 uint8_t a;

 if(argn < 255) {
  a = argn;
  log_buf_write(b, &a, 1);
 } else {
  a = 255;
  log_buf_write(b, &a, 1);
  log_buf_write(b, (const uint8_t*)&argn, 4);
 }
}

void 
logbuf_int32(logbuf_t *c, uint32_t argn, uint32_t i)
{
 uint8_t code = LOGBUF_T_I32;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&i, 4);
}

void 
logbuf_int64(logbuf_t *c, uint32_t argn, uint64_t i)
{
 uint8_t code = LOGBUF_T_I64;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&i, 8);
}

void 
logbuf_ptr(logbuf_t *c, uint32_t argn, void *i)
{
#if defined(ILP32)
 uint8_t code = LOGBUF_T_I32;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&i, 4);
#elif defined(LLP64) || defined(LP64)
 uint8_t code = LOGBUF_T_I64;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&i, 8);
#else
#error unknown data model
#endif
}

void 
logbuf_time(logbuf_t *c, uint32_t argn, uint64_t t)
{
 uint8_t code = LOGBUF_T_TIME;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&t, 8);
}

void 
logbuf_string(logbuf_t *c, uint32_t argn, const char *s)
{
 uint8_t code = LOGBUF_T_STR;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)s, strlen(s)+1);
}

void 
logbuf_fmtstrn(logbuf_t *c, uint32_t argn, const char *s)
{
 uint8_t code = LOGBUF_T_FMT;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)s, strlen(s)+1);
}

void 
logbuf_fmtstr(logbuf_t *c, const char *s)
{
 uint8_t code = LOGBUF_T_FMT;
 log_buf_write(c, &code, 1);
 code = 0;
 log_buf_write(c, &code, 1);
 log_buf_write(c, (uint8_t*)s, strlen(s)+1);
}

void 
logbuf_data(logbuf_t *c, uint32_t argn, const uint8_t *d, uint32_t l)
{
 uint8_t code = LOGBUF_T_DATA;
 log_buf_write(c, &code, 1);
 log_buf_put_arg(c, argn);
 log_buf_write(c, (uint8_t*)&l, 4);
 log_buf_write(c, d, l);
}

void 
logbuf_fmtauto(logbuf_t *b, uint32_t *argn, const char *fmt, ...)
{
 va_list ap;
 va_start(ap, fmt);
 logbuf_fmtauto_va(b, argn, fmt, ap);
 va_end(ap);
}

void 
logbuf_fmtauto_va(logbuf_t *b, uint32_t *argn, const char *fmt, va_list ap)
{
 char c;
 uint32_t n;
 int long_int;
 int array;

 n = argn != NULL ? *argn : 0;

 logbuf_fmtstrn(b, n++, fmt);
 while((c = *fmt++) != 0) {
  if(c != '%') continue;

  switch(*fmt) {
		  case '#':    fmt++;
					   n = 0;
					   while(*fmt >= '0' && *fmt <= '9')
						n = n*10 + (*fmt++ - '0');
                       if(*fmt == '#') fmt++; /* # used as separator, ignore */
					   break;
  }

  long_int = 0;
  array = 0;
  /* parse format modifiers */
  while(1) {
     switch(*fmt) {
           case 'l': long_int = 1;
                     fmt++;
                     continue;
           case ',': array = 1;
                     fmt++;
                     continue;
           case '+':
           case '0':
           case '1':
           case '2':
           case '3':
           case '4':
           case '5':
           case '6':
           case '7':
           case '8':
           case '9': fmt++;
                     continue;
           default: break;
     }
     break;
  }


  switch(*fmt++) {
	/* 
	 * %S and conditionals should not be supported,
	 */
		   case 's':    logbuf_string(b, n++, va_arg(ap, const char*));
						break;
		   case 'e':
                        logbuf_data(b, n++, va_arg(ap, uint8_t*), 6);
						break;
		   case 'a':
		   case 'x':
		   case 'X':
		   case 'u':
		   case 'd':    if(array) {
                         uint8_t *buf = va_arg(ap, uint8_t*);
                         uint32_t len = va_arg(ap, uint32_t);
                         logbuf_data(b, n++, buf, len);
                        } else if(long_int) logbuf_int64(b, n++, va_arg(ap, int64_t));
                        else logbuf_int32(b, n++, va_arg(ap, uint32_t));
						break;
		   case 'n':    n++; 
                        break;
		   case '%':    break;
		   default:     logbuf_assert_hook("logbuf_fmtauto_va: illegal format code: 0x%x", (uint32_t)(uint8_t)*(fmt-1));
  }
 }
 if(argn != NULL) *argn = n;
}

void 
logbuf_simple_message(logcounter_t *lc, uint64_t ev_type, uint32_t msg_id, const char *fmt, ...)
{
 logbuf_t *b = logbuf_get(lc, ev_type, msg_id);
 va_list ap;
 uint32_t n = 0;
 if(b != NULL) {
  va_start(ap, fmt);
  logbuf_fmtauto_va(b, &n, fmt, ap);
  va_end(ap);
  logbuf_send(b);
 }
}

void 
logbuf_debug(logcounter_t *lc, const char *fmt, ...)
{
 logbuf_t *b = logbuf_get(lc, LOGBUF_EV_DEBUG, LOGBUF_ID_DEBUG);
 va_list ap;
 uint32_t n = 0;
 if(b != NULL) {
  va_start(ap, fmt);
  logbuf_fmtauto_va(b, &n, fmt, ap);
  va_end(ap);
  logbuf_send(b);
 }
}
