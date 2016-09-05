#include <stdio.h>
#include "logcounter.h"
#include "logbuf.h"
#include "log_stdio.h"


int
main()
{
 logcounter_t *lc;
 struct _logdest *dest;
 lc = logcounter_create();
 dest = logdest_stdio_create(stdout, NULL);
 
 logcounter_connect(lc, dest);
 logcounter_set_filter(lc, dest, 1);

 logbuf_simple_message(lc, 1, 1, "hex %x HEX %X 0-fill %04x ' '-fill \"%6X\" u64 %020lx", 0x34, 0xa64, 0xbac, 0x8a, 0xfedcba9876543210ULL);
 logbuf_simple_message(lc, 1, 1, "ether %e ip %a", "\x00\x00\x16\x18\xab\xcd", 0xc0a80102);
 /* logcounter_disconnect(lc, dest); */
 logcounter_destroy(lc);
 logdest_stdio_destroy(dest);
 return 0;
}
