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

int 
logdest_get_arg(uint8_t *buf, uint32_t buflen, 
				uint8_t type, uint8_t argn, void** ptr, uint32_t *len, uint8_t *type_out, uint8_t *arg_out) 
{
 uint32_t dlen;
 uint8_t t;
 uint8_t n;
 uint8_t *bufend = buf + buflen;


 while(buf < bufend) {

  t = *buf++;
  if(t < 16) n = 255;
  else n = *buf++;
  
  if((argn == 0 || argn == n) && (type == 0 || t == type)) {
   switch (t) {
	/* FIXME: we do assume that there's enough room in buf for an argument */
		   case  LOGBUF_T_I64:
		   case  LOGBUF_T_TIME:
				   if(len) *len = 8;
				   if(ptr) *ptr = buf;
				   break;
		   case LOGBUF_T_STR:
		   case LOGBUF_T_FMT:
				   if(ptr) *ptr = buf;
				   if(len) *len = strlen((const char*)buf)+1;
				   break;
		   case LOGBUF_T_I32:
		   case LOGBUF_T_MID:
		   case LOGBUF_T_GRP:
		   case LOGBUF_T_RID:
				   if(len) *len = 4;
				   if(ptr) *ptr = buf;
				   break;
		   case LOGBUF_T_DATA:
				   if(ptr) *ptr = buf+4;
				   if(len) *len = logbuf_get32(buf);
				   break;
   }
   if(type_out) *type_out = t;
   if(arg_out) *arg_out = n;
   return 1;
  }

  switch(t) {
		   case LOGBUF_T_MID:
		   case LOGBUF_T_GRP:
		   case LOGBUF_T_RID:
		   case LOGBUF_T_I32: dlen=4; break;
		   case LOGBUF_T_I64:
		   case LOGBUF_T_TIME: dlen=8; break;
		   case LOGBUF_T_STR:
		   case LOGBUF_T_FMT: dlen=strlen((const char*)buf)+1; break;
		   case LOGBUF_T_DATA: dlen=4+logbuf_get32(buf); break;
  }
  buf += dlen;
 }
 return 0;
}
