/**
*
*  \file        log_fmt.c
*  \brief       Logbuf to string conversion.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/
#include <malloc.h>
#include <string.h>
#include "logdest.h"
#include "logres.h"
#include "logbuf.h"

#define FMT_STACK_SIZE 8

static uint8_t
fmt_char2type(char c)
{
 switch(c) {
         case 'D': return LOGBUF_T_I64;
         case 's': return LOGBUF_T_STR;
         case 'S': return LOGBUF_T_FMT;
         case 'b': return LOGBUF_T_DATA;
         case 'd': return LOGBUF_T_I32;
         case '*':
         default:  return 0;
 }
}

static int
fmt_skip(const char **fmt, uint32_t *a)
{
 const char *f = *fmt;
 uint32_t an = *a;
 unsigned d = 1;
 int rc = 1;
 while(*f && d) {
  if(*f++ == '%') {
   switch(*f) {
           case '(': d++; an++; break;
           case ')': d--; break;
           case '|': if(d==1) {d=0; an--;}
                      break;
           case 0: continue;
   }
   f++;
  }
 }
 *fmt = f;
 *a = an;
 return rc;
}

static uint64_t
load_uint(const void *d, int w)
{
 int i;
 uint64_t n;

 for(i = 0, n = 0; i < w; i++)
#ifdef LITTLE_ENDIAN
  n = (n<<8) + *((uint8_t*)d+(w-i-1));
#else
  n = (n<<8) + *((uint8_t*)d+i);
#endif

 return n;
}

static int64_t
load_int(const void *d, int w)
{
 int64_t n = load_uint(d, w);

 return n<<(8-w)*8>>(8-w)*8;
}

struct _strbuf {
 uint32_t used;
 uint32_t bufok;
 char *buf_start;
};


static struct _strbuf* prepare_buf(void)
{
 char *bufstart;
 struct _strbuf *b;
 bufstart = malloc(128+sizeof(struct _strbuf));
 if(bufstart == NULL) return NULL;
 b = (struct _strbuf*)(bufstart + 128);
 b->bufok = 1;
 b->buf_start = bufstart;
 b->used = 1;
 *b->buf_start = 0;
 return b;
}

static void putstrstr(void *out_buf, const char *str)
{
 struct _strbuf *b = *(struct _strbuf**)out_buf;
 uint32_t bufsz;
 uint32_t sl;
 uint32_t used;
 char *bufstart;

 if(!b->bufok) return;
 sl = strlen(str);

 bufstart = b->buf_start;
 bufsz = (char*)b - bufstart;
 used = b->used;

 if(bufsz - b->used < sl) {
  char *new_buf;
  do bufsz *= 2; while(bufsz - used < sl);
  new_buf = malloc(bufsz+sizeof(struct _strbuf));
  if(new_buf == NULL) {
   b->bufok = 0;
   return;
  }
  memcpy(new_buf, bufstart, used);
  free(bufstart);
  bufstart = new_buf;
  b = (struct _strbuf*)(bufstart + bufsz);
  b->buf_start = bufstart;
  b->bufok = 1;
  *(struct _strbuf**)out_buf = b;
 }
 memcpy(bufstart + used-1, str, sl+1);
 b->used = used + sl;
}

static const char*
get_fmtstr(struct _logres *res, const uint8_t *buf, uint32_t len)
{
 const char *fmtstr = NULL;

 if(res != NULL) {
  const void *mid_ptr;
  uint32_t mid;
  logdest_get_arg(buf, len, LOGBUF_T_MID, 0, &mid_ptr, NULL, NULL, NULL);
  if(mid_ptr) {
   mid = logbuf_get32(mid_ptr);
   fmtstr = logres_get(res, mid, "FMTSTR", NULL);
  }
 }
 if(fmtstr == NULL)
  logdest_get_arg(buf, len, LOGBUF_T_FMT, 0, (const void**)&fmtstr, NULL, NULL, NULL);

 return fmtstr;
}

void logdest_format_message(struct _logres *res, const char *fmtstr, const uint8_t *buf,
                uint32_t len, char **out)
{
 struct _strbuf *b;
 char fmtbuf[64];
 b = prepare_buf();
 if(b == NULL) {
  *out = NULL;
  return;
 }

 if(fmtstr == NULL)
  fmtstr = get_fmtstr(res, buf, len);

 if(fmtstr == NULL) {
  const void *mid_ptr;
  uint32_t mid = 0;

  logdest_get_arg(buf, len, LOGBUF_T_MID, 0, &mid_ptr, NULL, NULL, NULL);
  if(mid_ptr) mid = logbuf_get32(mid_ptr);
  else mid = 0;
  sprintf(fmtbuf, "<MID 0x%x: no format string>", mid);
  putstrstr(&b, fmtbuf);
 } else {
  logdest_format_message_stream(putstrstr, &b, res, fmtstr, buf, len);
 }
 if(!b->bufok) {
  free(b);
  *out = NULL;
  return;
 }
 *out = b->buf_start;
}

static void 
pad_int(void (*putstr)(void *out_buf, const char *str), void *out_buf,
        const char *v, uint32_t field_width, int zero_pad) {
 if(field_width) {
  uint32_t n;
  n = strlen(v);
  while(n++ < field_width)
   putstr(out_buf, zero_pad ? "0" : " ");
 }
}

static void 
put_padded_int(void (*putstr)(void *out_buf, const char *str), void *out_buf,
                const char *v, uint32_t field_width, int zero_pad, int hex_prefix) {
 if(hex_prefix) {
  if(!zero_pad) pad_int(putstr, out_buf, v, field_width >= 2 ? field_width-2 : field_width, zero_pad);
  putstr(out_buf, "0x");
  if(zero_pad) pad_int(putstr, out_buf, v, field_width >= 2 ? field_width-2 : field_width, zero_pad);
 } else {
  pad_int(putstr, out_buf, v, field_width, zero_pad);
 }
 putstr(out_buf, v);
}

static void
format_int(char *buf, char fmt, uint32_t data_width, const char *dataptr)
{
 const char *ff;
 uint64_t i;
 int sign = 0;

 switch(fmt) {
         case 'd': ff = "%lld";
                   sign = 1;
                   break;
         case 'u': ff = "%llu";
                   break;
         case 'x': ff = "%llx";
                   break;
         case 'X': ff = "%llX";
                   break;
 }
                                   
 if(!sign) i = load_uint(dataptr, data_width);
 else i = load_int(dataptr, data_width);

 sprintf(buf, ff, i);
}

void
logdest_format_message_stream(void (*putstr)(void *out_buf, const char *str), void *out_buf, struct _logres *res,
                const char *fmt, const uint8_t *buf, uint32_t len)
{
 const void *v;
 const char *p;
 const uint8_t *p1;
 uint32_t w, l;
 uint32_t i;
 uint32_t sp = 0;
 const char *stack[FMT_STACK_SIZE];
 char fmtbuf[128];
 uint32_t field_width;
 int zero_pad;
 int alt_form;
 uint32_t argn = 1;
 char f;

 if(fmt == NULL)
  fmt = get_fmtstr(res, buf, len);

 if(fmt == NULL) {
  const void *mid_ptr;
  uint32_t mid = 0;

  logdest_get_arg(buf, len, LOGBUF_T_MID, 0, &mid_ptr, NULL, NULL, NULL);
  if(mid_ptr) mid = logbuf_get32(mid_ptr);
  else mid = 0;
  sprintf(fmtbuf, "<MID 0x%x: no format string>", mid);
  putstr(out_buf, fmtbuf);
  return;
 }

  while(1) {
   if(*fmt == 0) {
    if(sp == 0) break;
    fmt = stack[--sp];
    continue;
   }


   if(*fmt == '%') {
    /* argument numbers and conditionals */
    fmt++;
    switch(*fmt) {
             case '(':
                       fmt++;
                       if(*fmt >= '0' && *fmt <= '9') {
                        argn = 0;
                        do argn = argn*10 + (*fmt++ - '0');
                        while(*fmt >= '0' && *fmt <= '9');
                       }
                       if(logdest_get_arg(buf, len, fmt_char2type(*fmt), argn, 0, 0, 0, 0)) fmt++;
                       else {fmt_skip(&fmt, &argn); argn++;}
                       continue;
             case '|': fmt++; fmt_skip(&fmt, &argn); continue;
             case ')': fmt++; continue;
    }

    zero_pad = 0;
    field_width = 0;
    alt_form = 0;

    /* parse format modifiers */
    while(1) {
     switch(*fmt) {
           case 'l': fmt++;
                     continue; /* this is only for fmt_simple, ignore */
           case '+': alt_form = 1;
                     fmt++;
                     continue; /* this is only for fmt_simple, ignore */
           case '#': fmt++;
                     argn = 0;
                     while(*fmt >= '0' && *fmt <= '9')
                      argn = argn*10 + (*fmt++ - '0');
                     if(*fmt == '#') fmt++; /* # used as separator, ignore */
                     continue;
           case '0': if(field_width == 0) zero_pad = 1;
           case '1':
           case '2':
           case '3':
           case '4':
           case '5':
           case '6':
           case '7':
           case '8':
           case '9': field_width = field_width*10 + *fmt-'0';
                     if(field_width > 4096) {
                      sprintf(fmtbuf,"<argn #%u: field width > 4096>", argn);
                      putstr(out_buf, fmtbuf);
                      field_width = 0;
                      break;
                     }
                     fmt++;
                     continue;
           default: break;
     }
     break;
    }

    /* format type characters */
    switch(f = *fmt) {
           case '%':
                   putstr(out_buf, "%");
                   break;

           case 'S':
                   if(logdest_get_arg(buf, len, LOGBUF_T_FMT, argn++, (const void**)&p, 0, 0, 0)) {
                    if(sp >= FMT_STACK_SIZE) {
                     sprintf(fmtbuf, "<arg #%u: fmt stack overflow>", argn-1);
                     putstr(out_buf, fmtbuf);
                     break;
                    }
                    stack[sp++] = fmt+1;
                    fmt = p;
                    continue;
                   }
                   sprintf(fmtbuf, "<arg #%u: no format string>", argn-1);
                   putstr(out_buf, fmtbuf);
                   break;

           case 's':
                   if(logdest_get_arg(buf, len, LOGBUF_T_STR, argn++, (const void**)&p, 0, 0, 0)) {
                    putstr(out_buf, p);
                   } else {
                    sprintf(fmtbuf,"<arg #%u: no string>", argn-1);
                    putstr(out_buf, fmtbuf);
                   }
                   break;
           case 'd':
           case 'u': alt_form = 0;
           case 'x':
           case 'X':
                   if(logdest_get_arg(buf, len, LOGBUF_T_I32, argn, &v, 0, 0, 0)) {
                    format_int(fmtbuf, f, 4, v);
                   } else if(logdest_get_arg(buf, len, LOGBUF_T_I64, argn, &v, 0, 0, 0)) {
                    format_int(fmtbuf, f, 8, v);
                   } else {
                    sprintf(fmtbuf, "<arg #%u: no integer>", argn);
                    putstr(out_buf, fmtbuf);
                    argn++;
                    break;
                   }
                   put_padded_int(putstr, out_buf, fmtbuf, field_width, zero_pad, alt_form);
                   argn++;
                   break;
           case 'e':
                   if(!logdest_get_arg(buf, len, LOGBUF_T_DATA, argn++, (const void**)&p1, &l, 0, 0)) {
                    sprintf(fmtbuf, "<arg #%u: no data>", argn-1);
                    putstr(out_buf, fmtbuf);
                    break;
                   }
                   if(l != 6) {
                    sprintf(fmtbuf, "<arg #%u: len is not 6>", argn-1);
                    putstr(out_buf, fmtbuf);
                    break;
                   }
                   sprintf(fmtbuf, "%02x:%02x:%02x:%02x:%02x:%02x", (unsigned)p1[0], (unsigned)p1[1], (unsigned)p1[2],
                                   (unsigned)p1[3], (unsigned)p1[4], (unsigned)p1[5]);
                   putstr(out_buf, fmtbuf);
                   break;
           case 'a':
                   if(logdest_get_arg(buf, len, LOGBUF_T_I32, argn++, &v, 0, 0, 0)) {
                    i = logbuf_get32(v);
                    sprintf(fmtbuf, "%d.%d.%d.%d", i>>24, (i>>16)&0xff, (i>>8)&0xff, i&0xff);
                   } else {
                    sprintf(fmtbuf, "<arg #%u: no integer>", argn-1);
                   }
                   putstr(out_buf, fmtbuf);
                   break;
           case ',':
                   w = fmt[1] >= '1' && fmt[1] <= '8' ? *++fmt - '0' : 4;

                   if(!logdest_get_arg(buf, len, LOGBUF_T_DATA, argn++, (const void**)&p, &l, 0, 0)) {
                    sprintf(fmtbuf, "<arg #%u: no data>", argn-1);
                    putstr(out_buf, fmtbuf);
                    break;
                   }

                   if(l%w) {
                    sprintf(fmtbuf, "<arg #%u: length is not a multiple of %d>", argn-1, w);
                    putstr(out_buf, fmtbuf);
                    break;
                   }

                   switch(f = *++fmt) {
                           case 'd':
                           case 'u':
                                     alt_form = 0;
                           case 'x':
                           case 'X':
                                  for(i = 0; i < l; i+=w) {
                                   if(i > 0) putstr(out_buf, " ");
                                   format_int(fmtbuf, f, w, p+i);
                                   put_padded_int(putstr, out_buf, fmtbuf, field_width, zero_pad, alt_form);
                                  }
                                  break;
                           case 'a':
                                  if(w != 4) {
                                   sprintf(fmtbuf, "<arg #%u: ip list should have width specifier 4, not %d>", argn-1, w);
                                   putstr(out_buf, fmtbuf);
                                   break;
                                  }
                                  for(i = 0; i < l; i+=w) {
                                   uint32_t n = logbuf_get32(p+i);
                                   sprintf(fmtbuf, "%s%d.%d.%d.%d", i>0?" ":"",
                                                   n>>24, (n>>16)&0xff, (n>>8)&0xff, n&0xff);
                                   putstr(out_buf, fmtbuf);
                                  }
                                  break;

                           default:
                                  sprintf(fmtbuf, "<arg #%u: unknown format char-'%c'>", argn, *fmt);
                                  putstr(out_buf, fmtbuf);
                                  if(*fmt == 0) continue;
                   }
                   break;
           case 'n': argn++; break;
           default:
                   sprintf(fmtbuf, "<arg #%u: unknown format char-'%c'>", argn, *fmt);
                   putstr(out_buf, fmtbuf);
                   if(*fmt == 0) continue; /* skip fmt++ */

    }
   } else {
    fmtbuf[0] = *fmt;
    fmtbuf[1] = 0;
    putstr(out_buf, fmtbuf);
   }
   fmt++;
  }
}

