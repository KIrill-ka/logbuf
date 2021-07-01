/**
*
*  \file        logdest.h
*  \brief       Provides log destinations with helper functions to process logbufs.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/
#ifndef LOGDEST_H
#define LOGDEST_H
#include <log_inttypes.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct _logdest {
 int (*ld_send)(struct _logdest *ld, const uint8_t *buf, uint32_t len);
};
typedef struct _logdest logdest_t;
struct _logres;

extern int logdest_get_arg(const uint8_t *buf, uint32_t buflen, uint8_t type, uint32_t argn, 
                const void** ptr, uint32_t *len, uint8_t *type_out, uint32_t *arg_out);
extern void logdest_format_message_stream(void (*putstr)(void *out_buf, const char *str), void *out_buf, struct _logres *res,
                const char *fmt, const uint8_t *buf, uint32_t len);
extern void logdest_format_message(struct _logres *res, const char *fmt, const uint8_t *buf, uint32_t len, char **out);

#ifdef  __cplusplus
}
#endif
#endif
