#include <stdio.h>
#include "logdest.h"
#include "log_stdio.h"
#include "logcounter.h"
#include "logbuf.h"

int main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 struct _logdest *dest;

 lc = logcounter_create();
 dest = logdest_stdio_create(stdout, NULL);
 logcounter_connect(lc, dest);
 logcounter_set_filter(lc, dest, 1);

 printf("test#10.1 arg1=1, arg1000=1000, arg1001=1001, arg1000000=1000000\n");
 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_fmtstr(lb, "test#10.2 arg1=%u, arg1000=%#1000u, arg1001=%u, arg1000000=%#1000000u");
  logbuf_int32(lb, 1, 1);
  logbuf_int32(lb, 1000, 1000);
  logbuf_int32(lb, 1001, 1001);
  logbuf_int32(lb, 1000000, 1000000);
  logbuf_send(lb);
 }
 return 0;
}
