#include <stdio.h>
#include "logdest.h"
#include "logcounter.h"
#include "logbuf.h"


static void print_str(void *b, const char *str)
{
 printf("%s", str);
}

static int print_message(logdest_t *ld, const uint8_t *buf, uint32_t len)
{
  logdest_format_message_stream(print_str, NULL, NULL, NULL, buf, len);
  printf("\n");
  return 1;
}

logdest_t simple_dest = {
 print_message
};

void sub1(logbuf_t *lb, uint32_t argn)
{
 if(lb != NULL) logbuf_fmtauto(lb, &argn, "sub%u()", 1);
}

void sub2(logbuf_t *lb, uint8_t argn)
{
 /* sub2 is silent */
}

void sub3_1(logbuf_t *lb, uint32_t argn)
{
 if(lb != NULL) logbuf_fmtauto(lb, &argn, "sub%u()", 31);
}

void sub3(logbuf_t *lb, uint32_t argn)
{
 if(lb != NULL) {
  logbuf_fmtstrn(lb, argn, "sub3(%(S%S%))");
 }
 sub3_1(lb, argn+1);
}

struct t {
 struct t *left;
 struct t *right;
 int n;
};

struct t n10 = {0, 0, 10};
struct t n11 = {0, 0, 11};
struct t n9 = {0, 0, 9};
struct t n8 = {0, 0, 8};
struct t n7 = {0, &n8, 7};
struct t n6 = {&n9, 0, 6};
struct t n5 = {&n10, &n11, 5};
struct t n4 = {0, 0, 4};
struct t n3 = {&n4, &n5, 3};
struct t n2 = {&n6, &n7, 2};
struct t tree = {&n3, &n2, 0};

void tree_print_rec(logbuf_t *lb, struct t *t, uint8_t *argc)
{
  if(!t) {
   logbuf_int32(lb, (*argc)++, 0);
   return;
  }
  if(!t->left && !t->right) logbuf_fmtstrn(lb, (*argc)++, "%d");
  else logbuf_fmtstrn(lb, (*argc)++, "%d (%(S%S%|%n-%), %(S%S%|%n-%))");
  logbuf_int32(lb, (*argc)++, t->n);
  if(t->left || t->right) {
   tree_print_rec(lb, t->left, argc);
   tree_print_rec(lb, t->right, argc);
  }
}

int main()
{
 logcounter_t *lc;
 logbuf_t *lb;
 lc = logcounter_create();
 logcounter_connect(lc, &simple_dest);
 logcounter_set_filter(lc, &simple_dest, 1);

 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_fmtstr(lb, "test#30.2 simple nested formatting: %s(%S)");
  logbuf_string(lb, 1, "main");
 }
 sub1(lb, 2);
 printf("test#30.1 simple nested formatting: main(sub1())\n");
 if(lb != NULL) logbuf_send(lb);

 lb = logbuf_get(lc, 1, 1);
 if(lb != NULL) {
  logbuf_fmtstr(lb, "test#31.2 complex nested formatting: %s(%(10S%S%)%(20S, %S%)%(30S, %S%))");
  logbuf_string(lb, 1, "main");
 }
 sub1(lb, 10);
 sub2(lb, 20);
 sub3(lb, 30);
 printf("test#31.1 complex nested formatting: main(sub1(), sub3(sub31()))\n");
 if(lb != NULL) logbuf_send(lb);

 { 
  /* NOTE: this is not for general use, because formatting stack depth is 8 and arg number is uint8 */
  uint8_t argn = 1;
  lb = logbuf_get(lc, 1, 1);
  if(lb != NULL) {
   logbuf_fmtstr(lb, "test#32.2 binary tree: %(S%S%)");
   tree_print_rec(lb, &tree, &argn);
   printf("test#32.1 binary tree: 0 (3 (4, 5 (10, 11)), 2 (6 (9, -), 7 (-, 8)))\n");
   logbuf_send(lb);
  }
 }
 return 0;
}
