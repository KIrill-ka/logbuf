#include <stdio.h>
#include "logdest.h"
#include "logcounter.h"
#include "logbuf.h"


static void print_str(void *b, const char *str)
{
 printf("%s", str);
}

static int print_message(logdest_t *ld, const uint8_t *buf, uint32_t len)
{
  logdest_format_message_stream(print_str, NULL, NULL, NULL, buf, len);
  printf("\n");
  return 1;
}

logdest_t simple_dest = {
 print_message
};

int main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 lc = logcounter_create();
 logcounter_connect(lc, &simple_dest);
 logcounter_set_filter(lc, &simple_dest, 1);

 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_fmtstr(lb, "Hello World %d");
  logbuf_int32(lb, 1, 1);
  logbuf_send(lb);
 }
 return 0;
}
