/**
*
*  \file        logres.h
*  \brief       Logres provides log destinations with keyed text strings.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*  Resources are loaded from ini-style text files.
*/

#ifndef LOGRES_H
#define LOGRES_H

#include <log_inttypes.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct _logres;
typedef struct _logres logres_t;

typedef enum {
 LOGRES_PARSE_OK,
 LOGRES_PARSE_NO_INDEX,
 LOGRES_PARSE_BAD_NUMBER,
 LOGRES_PARSE_NO_EQ,
 LOGRES_PARSE_NO_MEM,
 LOGRES_PARSE_BAD_KEY
} logres_parse_status_t;

extern logres_parse_status_t logres_parse_ini(logres_t *r, const char *ini, uint32_t *err_pos);
extern const char *logres_get(logres_t *r, uint32_t id, const char *type_key, uint32_t *len);
extern struct _logres* logres_init(void);
extern void logres_free(logres_t *r);


#ifdef  __cplusplus
}
#endif
#endif
