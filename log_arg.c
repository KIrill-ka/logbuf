/**
*
*  \file        log_arg.c
*  \brief       Log buffer parser.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/

#include <string.h>
#include "logdest.h"
#include "logbuf.h"

static uint32_t 
strlen_safe(const uint8_t *s, uint32_t maxlen) /* same as GNU strnlen */
{
 uint32_t i;
 for(i = 0; i < maxlen; i++)
  if(*s++ == '\0') break;
 return i;
}

int 
logdest_get_arg(const uint8_t *buf, uint32_t buflen, 
				uint8_t type, uint32_t argn, const void** ptr, uint32_t *len,
                uint8_t *type_out, uint32_t *arg_out) 
{
 uint32_t dlen;
 uint8_t t;
 uint32_t n;
 const uint8_t *bufend = buf + buflen;

 while(buf+1 < bufend) { /* 2 is the minimum length for tag+value */
  t = *buf++;
  if(t < 16) n = 0xFfffFfff;
  else {
   n = *buf++;
   if(n == 255) {
    if(bufend - buf < 4) return 0; /* no space for an extended argnum - corrupted buf */
    n = logbuf_get32(buf);
    buf += 4;
   }
  }
  
  switch(t) {
		   case LOGBUF_T_MID:
		   case LOGBUF_T_RID:
		   case LOGBUF_T_I32: dlen=4; break;
		   case LOGBUF_T_GRP:
		   case LOGBUF_T_I64:
		   case LOGBUF_T_TIME: dlen=8; break;
		   case LOGBUF_T_STR:
		   case LOGBUF_T_FMT: dlen=strlen_safe(buf, bufend-buf)+1; break;
		   case LOGBUF_T_DATA: 
                              if(bufend - buf < 4) return 0; /* no space for length - corrupted buf */
                              dlen = logbuf_get32(buf);
                              buf += 4;
                              break;
           default: return 0; /* unknown type - corrupted buf */
  }
  if(bufend - buf < dlen) return 0; /* no space for data - corrupted buff */

  if((argn == 0 || argn == n) && (type == 0 || t == type)) {
   if(len) *len = dlen;
   if(ptr) *ptr = buf;
   if(type_out) *type_out = t;
   if(arg_out) *arg_out = n;
   return 1;
  }

  buf += dlen;
 }
 /* if(buf != bufend) the buf is corrupted */
 return 0;
}
