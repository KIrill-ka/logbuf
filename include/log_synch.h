/**
*
*  \file        log_synch.h
*  \brief       Synchronisation primitive compatibility for logbuf.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/

#ifndef LOG_SYNCH_H
#define LOG_SYNCH_H
#if defined(__linux)
#include <pthread.h>
typedef pthread_mutex_t logbuf_mutex_t;
typedef pthread_cond_t logbuf_cvar_t;
static inline void logbuf_cvar_init(logbuf_cvar_t *cv) { pthread_cond_init(cv, 0); }
static inline void logbuf_cvar_destroy(logbuf_cvar_t *cv) { pthread_cond_destroy(cv); }
static inline void logbuf_cvar_wait(logbuf_cvar_t *cv, logbuf_mutex_t *m) { pthread_cond_wait(cv, m); }
static inline void logbuf_cvar_broadcast(logbuf_cvar_t *cv) { pthread_cond_broadcast(cv); }
static inline void logbuf_mutex_init(logbuf_mutex_t *m) { pthread_mutex_init(m, 0); }
static inline void logbuf_mutex_destroy(logbuf_mutex_t *m) { pthread_mutex_destroy(m); }
static inline void logbuf_mutex_enter(logbuf_mutex_t *m) { pthread_mutex_lock(m); }
static inline void logbuf_mutex_exit(logbuf_mutex_t *m) { pthread_mutex_unlock(m); }
#else
#error "OS is not supported"
#endif
#endif
