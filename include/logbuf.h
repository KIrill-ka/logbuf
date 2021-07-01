/**
*
*  \file        logbuf.h
*  \brief       Declares functions used to compose log messages.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/
#ifndef LOGBUF_H
#define LOGBUF_H

#include <log_inttypes.h>

#define LOGBUF_ID_MAX_USER_ID	0xffff0000	
#define LOGBUF_ID_LOST	0xffff0001
#define LOGBUF_ID_DEBUG	0xffff0002

#define LOGBUF_EV_DEBUG 0x8000000000000000ULL

/* Data types stored in log buffer.
   Types with LOG_BUF_T < 16 do not allow arg number. */
#define LOGBUF_T_MID 	1	/* message id */
#define LOGBUF_T_GRP 	3	/* group aka ev_type */

#define LOGBUF_T_I32 	16  /* signed/unsigned int32, IPv4 */
#define LOGBUF_T_I64 	17	/* signed/unsigned int64 */
#define LOGBUF_T_STR 	18  /* c-string */
#define LOGBUF_T_DATA	19	/* variable length byte array */
#define LOGBUF_T_FMT 	20	/* format string */
#define LOGBUF_T_TIME	21	/* timestamp as uint64_t */
#define LOGBUF_T_RID 	22	/* resource identifier = string from _logres */

#ifdef  __cplusplus
extern "C" {
#endif

struct _logbuf;
typedef struct _logbuf logbuf_t;
struct _logcounter;

extern logbuf_t* logbuf_get(struct _logcounter *lc, uint64_t ev_type, uint32_t id);
extern void logbuf_send(logbuf_t *b);
extern void logbuf_free(logbuf_t *b);
extern int logbuf_event_needed(struct _logcounter *lc, uint64_t ev_type);
extern const uint8_t* logbuf_raw(logbuf_t *b, uint32_t *len);
extern void logbuf_assign_counter(logbuf_t *lb, struct _logcounter *lc);
extern logbuf_t* logbuf_create(uint64_t ev_type, uint32_t msg_id, int add_timestamp);
extern logbuf_t* logbuf_regenerate(uint8_t *src_buf, uint32_t len);

extern void logbuf_int32(logbuf_t *b, uint32_t argn, uint32_t i); 
extern void logbuf_int64(logbuf_t *b, uint32_t argn, uint64_t i); 
extern void logbuf_ptr(logbuf_t *b, uint32_t argn, void *i); 
extern void logbuf_string(logbuf_t *b, uint32_t argn, const char *str);
extern void logbuf_data(logbuf_t *b, uint32_t argn, const uint8_t *d, uint32_t l);
extern void logbuf_fmtstr(logbuf_t *b, const char *fmt);
extern void logbuf_fmtstrn(logbuf_t *b, uint32_t argn, const char *fmt);
extern void logbuf_time(logbuf_t *b, uint32_t argn, uint64_t t);
extern void logbuf_fmtauto(logbuf_t *b, uint32_t *argn, const char *fmt, ...);
#ifdef LOGBUF_VA /* logbuf_fmtauto_va requres stdarg.h header */
extern void logbuf_fmtauto_va(logbuf_t *b, uint32_t *argn, const char *fmt, va_list ap);
#endif

extern void logbuf_debug(struct _logcounter *lc, const char *fmt, ...);
extern void logbuf_simple_message(struct _logcounter *lc, uint64_t ev_type, uint32_t msg_id, 
                const char *fmt, ...);

extern void (*logbuf_assert_hook)(const char *fmt, ...);

#ifdef  __cplusplus
}
#endif

#endif
