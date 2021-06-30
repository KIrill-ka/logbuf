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

 printf("integer format\n");
 printf("test#02.1 %s\n", "%x 34 %X A64 %04x 0bac %6X \"    8A\" %020lx 0000fedcba9876543210");
 logbuf_simple_message(lc, 1, 1, "test#02.2 %%x %x %%X %X %%04x %04x %%6X \"%6X\" %%020lx %020lx", 0x34, 0xa64, 0xbac, 0x8a, 0xfedcba9876543210ULL);
 printf("address format\n");
 printf("test#03.1 %s\n", "%e 00:00:16:18:ab:cd %a 192.168.1.2");
 logbuf_simple_message(lc, 1, 1, "test#03.2 %%e %e %%a %a", "\x00\x00\x16\x18\xab\xcd", 0xc0a80102);
 {
    int32_t int_array[] = {1, 2, 3, 4};
    int32_t int_array1[] = {1, 2, 3, -4};
 	printf("arrays\n");
 	printf("%s\n", "test#04.1 %,d 1 2 3 4 %,1x a b c %02,1x 0a 0b 0c %+04,1x 0x0a 0x0b 0x0c %02,1x 0f f0 0f ff");
 	logbuf_simple_message(lc, 1, 1, "test#04.2 %%,d %,d %%,1x %,1x %%02,1x %02,1x %%+04,1x %+04,1x %%02,1x %02,1x", int_array, sizeof(int_array), 
                                    "\x0a\x0b\x0c", 3, "\x0a\x0b\x0c", 3, "\x0a\x0b\x0c", 3,
                                    "\x0f\xf0\x0f\xff", 4);

 	printf("test#05.1 %s\n", "%,d 1 2 3 -4");
 	logbuf_simple_message(lc, 1, 1, "test#05.2 %%,d %,d", int_array1, sizeof(int_array1));
 }

 {
   printf("conditions\n");
   const char *s = "%(2s%s%|%#1s:%u%) %s";
   logbuf_t *lb;
   printf("test#06.1 sw1:5 end\n");
   lb = logbuf_get(lc, 1, 1);
   logbuf_fmtstr(lb, s);
   logbuf_string(lb, 3, "end");
   logbuf_int64(lb, 1, 255);
   logbuf_int32(lb, 2, 5);
   logbuf_string(lb, 1, "sw1");
   printf("test#06.2 ");
   logbuf_send(lb);

   printf("test#07.1 port1 end\n");
   lb = logbuf_get(lc, 1, 1);
   logbuf_fmtstr(lb, s);
   logbuf_int64(lb, 1, 255);
   logbuf_int32(lb, 2, 5);
   logbuf_string(lb, 1, "sw1");
   logbuf_string(lb, 2, "port1");
   logbuf_string(lb, 3, "end");
   printf("test#07.2 ");
   logbuf_send(lb);
 }
 /* logcounter_disconnect(lc, dest); */
 logcounter_destroy(lc);
 logdest_stdio_destroy(dest);
 return 0;
}
