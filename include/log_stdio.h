/**
*
*  \file        log_stdio.h
*  \brief       stdio log destination
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/
#ifndef LOG_STDIO_H
#define LOG_STDIO_H
#include <log_inttypes.h>

#ifdef  __cplusplus
extern "C" {
#endif
struct _logdest;
struct _logres;

extern struct _logdest* logdest_stdio_create(FILE *out, struct _logres *res);
extern void logdest_stdio_destroy(struct _logdest *ld);

#ifdef  __cplusplus
}
#endif
#endif
