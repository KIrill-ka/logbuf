#include <stdio.h>
#include "logcounter.h"
#include "logbuf.h"
#include "logres.h"
#include "logdest.h"
#include "log_stdio.h"

logres_t *res;

const char * ini_buf = 
 "[MESSAGE1]\n"
 "INDEX = 0x1\n"
 "FMTSTR = test#40.2 Hello world %d\n" ;

int main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 logdest_t *dest;
 lc = logcounter_create();
 res = logres_init();
 logres_parse_ini(res, ini_buf, NULL);
 dest = logdest_stdio_create(stdout, res);
 logcounter_connect(lc, dest);
 logcounter_set_filter(lc, dest, 1);

 printf("test#40.1 Hello world 1\n");
 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_int32(lb, 1, 1);
  logbuf_send(lb);
 }
 return 0;
}
