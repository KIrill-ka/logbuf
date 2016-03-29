/**
*
*  \file        log_res.c
*  \brief       Resource file parser.
*  \author      Kirill A. Kornilov
*  \date        2016
*
*/

#include <stdlib.h>
#include <string.h>

#include "logres.h"

#define LOGRES_HASH_SIZE 256

#define LOGRES_HASH(x) (((x>>24)^(x>>16)^(x>>8)^x) & 0xff)

struct _logres_item {
 struct _logres_item* next;
 uint32_t resid;
 char *key;
 uint8_t *resbuf;
 uint32_t len;
};

struct _logres {
 struct _logres_item* hash[LOGRES_HASH_SIZE];
};

struct _logres*
logres_init(void)
{
 struct _logres *r;
 r = malloc(sizeof(*r));
 if(r == NULL) return NULL;
 memset(r, 0, sizeof(*r));
 return r;
}

void logres_free(logres_t *r)
{
 int i;
 struct _logres_item *x;
 for(i = 0; i < LOGRES_HASH_SIZE; i++)
  while((x = r->hash[i]) != NULL) {
   r->hash[i] = x->next;
   free(x->key);
   free(x->resbuf);
   free(x);
  }
 free(r);
}

const char*
logres_get(logres_t *r, uint32_t id, const char *type_key, uint32_t *len)
{
 struct _logres_item *i;
 for(i = r->hash[LOGRES_HASH(id)]; i != NULL; i = i->next)
  if(i->resid == id && !strcmp(i->key, type_key)) break;
 if(i == NULL) return NULL;
 if(len != NULL) *len = i->len;
 return (char*)i->resbuf;
}


static const char *find_next_section(const char *p)
{
  const char *start = p;
  while(1) {
   while(*p != '[' && *p != 0) p++;
   if(*p == 0) return NULL;
   if(p != start && p[-1] != '\n') {
    p++;
    continue;
   }
   while(*p != ']' && *p != '\n' && *p != '\r' && *p != 0) p++;
   if(*p != ']') continue;
   do p++; while(*p == ' ' || *p == '\t');
   if(*p != '\n' && *p != '\r') continue;
   return p;
  }
}

static int
alpha(char x) {
 return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z');
}

static logres_parse_status_t
get_next_token_value(const char **key_start, const char **key_end,
                const char **value_start, const char **value_end)
{
  const char *p = *value_end;
  do {
   while( *p != '\n' && *p != '\r' && *p != 0) p++;
   if(*p == 0) {
    *value_end = NULL;
    return LOGRES_PARSE_OK;
   }
   while(*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t') p++;
   if(*p == 0) {
    *value_end = NULL;
    return LOGRES_PARSE_OK;
   }
   if(*p == '[') {
    *value_end = NULL;
    return LOGRES_PARSE_OK;
   }
  } while (*p == '!');
  *key_start = p;
  if(!alpha(*p) && *p != '_') {
   *value_end = p;
   return LOGRES_PARSE_BAD_KEY;
  }
  while(alpha(*p) || *p == '_' || (*p >= '0' && *p <= '9')) p++;
  *key_end = p;
  while(*p == ' ' || *p == '\t') p++;
  if(*p != '=') {
   *value_end = p;
   return LOGRES_PARSE_NO_EQ;
  }
  p++;
  if(*p == ' ') p++; /* skip one space  */
  *value_start = p;
  while( *p != '\n' && *p != '\r' && *p != 0) p++;
  *value_end = p;
  return LOGRES_PARSE_OK;
}

static int
cmp_key(const char *s, const char *e, const char *key)
{
 uint32_t l;
 l = strlen(key);
 if(l != e-s) return 0;
 return memcmp(s, key, l) == 0;
}

static logres_parse_status_t
add_res(logres_t *r, uint32_t index, char *key, uint8_t *val, uint32_t len)
{
 struct _logres_item *i;
 uint8_t h;
 i = malloc(sizeof(*i));
 if(i == NULL) return LOGRES_PARSE_NO_MEM;
 i->resid = index;
 i->key = key;
 i->resbuf = val;
 i->len = len;
 h = LOGRES_HASH(index);
 i->next = r->hash[h];
 r->hash[h] = i;
 return LOGRES_PARSE_OK;
}

static logres_parse_status_t
add_res_string(logres_t *r, uint32_t index,
                const char *ks, const char *ke,
                const char *vs, const char *ve)
{
 char *k;
 uint8_t *v;
 logres_parse_status_t s;

 k = malloc(ke-ks+1);
 if(k == NULL) return LOGRES_PARSE_NO_MEM;
 v = malloc(ve-vs+1);
 if(v == NULL) {
  free(k);
  return LOGRES_PARSE_NO_MEM;
 }
 memcpy(k, ks, ke-ks);
 k[ke-ks] = 0;
 memcpy(v, vs, ve-vs);
 v[ve-vs] = 0;
 s = add_res(r, index, k, v, ve-vs+1);
 if(s != LOGRES_PARSE_OK) {
  free(k);
  free(v);
  return s;
 }
 return LOGRES_PARSE_OK;
}

logres_parse_status_t
logres_parse_ini(logres_t *r, const char *ini, uint32_t *err_pos)
{
 const char *section_pos;
 char *num_end;
 unsigned long index;
 const char *k_s, *k_e, *v_s, *v_e;
 logres_parse_status_t st;

 section_pos = ini;
 while(1) {
  section_pos = find_next_section(section_pos);
  if(section_pos == NULL) return LOGRES_PARSE_OK;
  v_e = section_pos;
  while(1) {
   st = get_next_token_value(&k_s, &k_e, &v_s, &v_e);
   if(st != LOGRES_PARSE_OK) {
    if(err_pos != NULL) *err_pos = v_e-ini;
    return st;
   }
   if(v_e == NULL) break;
   if(cmp_key(k_s, k_e, "INDEX")) break;
  }
  if(v_e == NULL) {
   if(err_pos != NULL) *err_pos = section_pos-ini;
   return LOGRES_PARSE_NO_INDEX;
  }
  index = strtoul(v_s, &num_end, 16);
  if(num_end != v_e) {
   if(err_pos != NULL) *err_pos = num_end-ini;
   return LOGRES_PARSE_BAD_NUMBER;
  }
  v_e = section_pos;
  while(1) {
   st = get_next_token_value(&k_s, &k_e, &v_s, &v_e);
   if(st != LOGRES_PARSE_OK) {
    if(err_pos != NULL) *err_pos = v_e-ini;
    return st;
   }
   if(v_e == NULL) break;
   if(cmp_key(k_s, k_e, "INDEX")) continue;
   st = add_res_string(r, index, k_s, k_e, v_s, v_e);
   if(st != LOGRES_PARSE_OK) {
    if(err_pos != NULL) *err_pos = v_e-ini;
    return st;
   }
  }
 }
}
