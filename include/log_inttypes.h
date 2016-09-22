/**
*
*  \file        log_inttypes.h
*  \brief       Fixed size integers, basic system compatibility macros.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/

#ifndef LOGINTTYPES_H
#define LOGINTTYPES_H
#if defined(__linux)
#include <stdint.h>
#include <endian.h>
#endif
#ifndef NULL
#define NULL 0
#endif

static inline void logbuf_put32b(uint32_t x, void *a)
{ 
                 uint8_t *addr = (uint8_t*) a;

                 addr[0] = (x & 0xff000000)>>24;
                 addr[1] = (x & 0x00ff0000)>>16;
                 addr[2] = (x & 0x0000ff00)>>8;
                 addr[3] = (x & 0x000000ff);
}

static inline void logbuf_put32l(uint32_t x, void *a)
{ 
                 uint8_t *addr = (uint8_t*) a;

                 addr[3] = (x & 0xff000000)>>24;
                 addr[2] = (x & 0x00ff0000)>>16;
                 addr[1] = (x & 0x0000ff00)>>8;
                 addr[0] = (x & 0x000000ff);
}

static inline uint32_t logbuf_get32b(void *a)
{ 
                 uint8_t *addr = (uint8_t*) a;

                 return addr[3] 
                         + ((uint32_t) addr[2] << 8)
                         + ((uint32_t) addr[1] << 16)
                         + ((uint32_t) addr[0] << 24);
}

static inline uint32_t logbuf_get32l(void *a)
{ 
                 uint8_t *addr = (uint8_t*) a;

                 return addr[0] 
                         + ((uint32_t) addr[1] << 8)
                         + ((uint32_t) addr[2] << 16)
                         + ((uint32_t) addr[3] << 24);
}

#if !defined(LOGBUF_BIG_ENDIAN) && !defined(LOGBUF_LITTLE_ENDIAN) && defined(__BYTE_ORDER)
/* byteorder detection using endian.h */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LOGBUF_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define LOGBUF_BIG_ENDIAN
#else
#error "unknow byte order"
#endif

#else

#if (!defined(LOGBUF_BIG_ENDIAN) && !defined(LOGBUF_LITTLE_ENDIAN)) || (defined(LOGBUF_BIG_ENDIAN) && defined(LOGBUF_LITTLE_ENDIAN))
#error "please, define either LOGBUF_BIG_ENDIAN or LOGBUF_LITTLE_ENDIAN"
#endif

#endif


#ifdef LOGBUF_LITTLE_ENDIAN

/**
* \brief Write 32 bit integer into portentialy unaligned buffer.
*/
static inline void logbuf_put32(uint32_t x, void *addr) { logbuf_put32l(x, addr); }
static inline uint32_t logbuf_get32(void *addr) {return logbuf_get32l(addr);}

#else
static inline void logbuf_put32(uint32_t x, void *addr) { logbuf_put32b(x, addr); }
static inline uint32_t logbuf_get32(void *addr) {return logbuf_get32b(addr);}
#endif

#endif
