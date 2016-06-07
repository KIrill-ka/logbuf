#include <stdio.h>
#include <string.h>
#include <time.h>
#include "logdest.h"
#include "logcounter.h"
#include "logbuf.h"
#include "logres.h"


static void print_str(void *b, const char *str)
{
 printf("%s", str);
}

const char * ini_buf = 
 "[MESSAGE1]\n"
 "INDEX = 0x1\n"
 "SYSLOG_LEVEL = DEBUG\n" ;

#define LOG_SRC_FILE_ARG_N 128
#define LOG_SRC_LINE_ARG_N 129

#define MSG_ID_1 1
#define MSG_GRP_1 1

#define LOG_SRC_INFO(lb) \
  logbuf_string((lb), LOG_SRC_FILE_ARG_N, __FILE__); \
  logbuf_int32((lb), LOG_SRC_LINE_ARG_N, __LINE__)

static int print_message(logdest_t *ld, uint8_t *buf, uint32_t len);
struct _my_logdest {
  logdest_t dest;
  logres_t *res;
} my_dest = {
 .dest = {.ld_send = print_message}
};

static const char *mask2grp(uint32_t mask)
{
 if((mask & MSG_GRP_1) != 0) return "GRP1";
 return "UNKNOWN";
}

static int print_message(logdest_t *ld, uint8_t *buf, uint32_t len)
{
  struct _my_logdest *d = (struct _my_logdest*)ld;
  uint64_t *ts_ptr;
  const char *loglev;
  char *file;
  uint32_t *line;
  uint32_t *midp;
  uint32_t *grp;
  char time_buf[128];
  time_t t;

  logdest_get_arg(buf, len, LOGBUF_T_MID, 0, (void**)&midp, NULL, NULL, NULL);
  loglev =  logres_get(d->res, *midp, "SYSLOG_LEVEL", NULL);
  logdest_get_arg(buf, len, LOGBUF_T_TIME, 0, (void**)&ts_ptr, NULL, NULL, NULL);
  logdest_get_arg(buf, len, LOGBUF_T_STR, LOG_SRC_FILE_ARG_N, (void**)&file, NULL, NULL, NULL);
  logdest_get_arg(buf, len, LOGBUF_T_I32, LOG_SRC_LINE_ARG_N, (void**)&line, NULL, NULL, NULL);
  logdest_get_arg(buf, len, LOGBUF_T_GRP, 0, (void**)&grp, NULL, NULL, NULL);

  t = *ts_ptr/1000000000;
  ctime_r(&t, time_buf);
  time_buf[strlen(time_buf)-1] = 0;
  printf("[%s] %s %s %s:%u ", mask2grp(*grp), loglev, time_buf, file, *line); 
  logdest_format_message_stream(print_str, NULL, NULL, NULL, buf, len);
  printf("\n");
  return 1;
}

int
main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 logres_t *res;
  
 res = logres_init();
 logres_parse_ini(res, ini_buf, NULL);
 my_dest.res = res;

 lc = logcounter_create();
 logcounter_connect(lc, &my_dest.dest);
 logcounter_tstamp_onoff(lc, &my_dest.dest, 1);
 logcounter_set_filter(lc, &my_dest.dest, MSG_GRP_1);

 lb = logbuf_get(lc, MSG_GRP_1, MSG_ID_1);
 if(lb != NULL) {
  logbuf_fmtstr(lb, "Hello World %d");
  logbuf_int32(lb, 1, 1);
  LOG_SRC_INFO(lb);
  logbuf_send(lb);
 }
 return 0;
}
