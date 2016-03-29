/**
*
*  \file        log_stdio.c
*  \brief       logstdio directs messages to file stdout or stderr.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/
#include <stdio.h>
#include <malloc.h>
#include "logdest.h"
#include "logres.h"

struct _logdest_stdio {
 logdest_t dst;
 FILE *stream;
 logres_t *res;
};

static void print_str(void *b, const char *str)
{
 fprintf((FILE*)b, "%s", str);
}

static int print_message(logdest_t *ld, uint8_t *buf, uint32_t len)
{
 struct _logdest_stdio *d = (struct _logdest_stdio*) ld;

 logdest_format_message_stream(print_str, d->stream, d->res, NULL, buf, len);
 fprintf(d->stream, "\n");
 return 1;
}

logdest_t*
logdest_stdio_create(FILE *out, struct _logres *res)
{
 struct _logdest_stdio *d;
 d = malloc(sizeof(*d));
 if(d == NULL) return NULL;
 d->dst.ld_send = print_message;
 d->stream = out;
 d->res = res;
 return &d->dst;
}

void
logdest_stdio_destroy(logdest_t *ld)
{
 free(ld);
}
