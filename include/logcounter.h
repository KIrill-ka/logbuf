/**
*
*  \file        logcounter.h
*  \brief       Log counter filters and routes messages to their destinations.
*  \author      Kirill A. Kornilov
*  \date        2016
*/


#ifndef LOGCOUNTER_H
#define LOGCOUNTER_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <log_inttypes.h>


struct _logcounter;
struct _logdest;
typedef struct _logcounter logcounter_t;

extern int logcounter_connect(logcounter_t *lc, struct _logdest *h);
extern void logcounter_set_filter(logcounter_t *lc, struct _logdest *h, uint64_t f);
extern void logcounter_set_default_filter(logcounter_t *lc, uint64_t f);
extern void logcounter_tstamp_onoff(logcounter_t *lc, struct _logdest *h, int enable);
extern void logcounter_disconnect(logcounter_t *lc, struct _logdest *h);
extern logcounter_t *logcounter_create(void);
extern void logcounter_destroy(logcounter_t *lc);


#ifdef  __cplusplus
}
#endif

#endif
