#include <stdio.h>
#include "logdest.h"
#include "logcounter.h"
#include "logbuf.h"
#include "logres.h"



logres_t *res;

static int print_message(logdest_t *ld, uint8_t *buf, uint32_t len)
{
  char *msg;
  logdest_format_message(res, NULL, buf, len, &msg);
  printf("%s\n", msg);
  return 1;
}

logdest_t simple_dest = {
 print_message
};

const char * ini_buf = 
 "[MESSAGE1]\n"
 "INDEX = 0x1\n"
 "FMTSTR = Hello world %d\n" ;

main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 lc = logcounter_create();
 logcounter_connect(lc, &simple_dest);
 logcounter_set_filter(lc, &simple_dest, 1);
 res = logres_init();
 logres_parse_ini(res, ini_buf, NULL);

 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_int32(lb, 1, 1);
  logbuf_send(lb);
 }
}
