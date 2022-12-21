#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Note: change to header file of your implemnted heap!
// #include "../runtime/gc/heap/heap.h"
#include "../runtime/gc/heap/derived_heap.h"

#ifndef EXTERNC
#define EXTERNC extern "C"
#endif

#define TIGER_HEAP_SIZE (1 << 20)

EXTERNC int tigermain(int);
gc::TigerHeap *tiger_heap = nullptr;

#define CHECK_HEAP                                                             \
  do {                                                                         \
    if (!tiger_heap) {                                                         \
      fprintf(stderr, "Warning: Allocate before initialization!");             \
      exit(-1);                                                                \
    }                                                                          \
  } while (0)

// Global interface & heap object to expose to runtime.c
EXTERNC char *Alloc(uint64_t size) {
  CHECK_HEAP;
  return tiger_heap->Allocate(size);
};
EXTERNC void GC(uint64_t size) {
  CHECK_HEAP;
  tiger_heap->GC();
}

EXTERNC uint64_t Used() {
  CHECK_HEAP;
  return tiger_heap->Used();
}

EXTERNC uint64_t MaxFree() {
  CHECK_HEAP;
  return tiger_heap->MaxFree();
}

EXTERNC long *init_array(int size, long init) {
  GET_RBP(tiger_heap->stack);
  (tiger_heap->stack) += 2;

  int i;
  uint64_t allocate_size = size * sizeof(long);
  long *a = (long *)tiger_heap->AllocateArray(allocate_size);
  if (!a) {
    tiger_heap->GC();
    a = (long *)tiger_heap->AllocateArray(allocate_size);
  }
  for (i = 0; i < size; i++)
    a[i] = init;
  return a;
}

struct string {
  int length;
  unsigned char chars[1];
};

EXTERNC int *alloc_record(int size, struct string *descriptor) {
  GET_RBP(tiger_heap->stack);
  (tiger_heap->stack) += 2;

  int i;
  int *p, *a;
  p = a = (int *)tiger_heap->AllocateRecord(size, descriptor->chars,
                                            descriptor->length);
  if (!p) {
    tiger_heap->GC();
    p = a = (int *)tiger_heap->AllocateRecord(size, descriptor->chars,
                                              descriptor->length);
  }
  for (i = 0; i < size; i += sizeof(int))
    *p++ = 0;
  return a;
}

EXTERNC int string_equal(struct string *s, struct string *t) {
  int i;
  if (s == t)
    return 1;
  if (s->length != t->length)
    return 0;
  for (i = 0; i < s->length; i++)
    if (s->chars[i] != t->chars[i])
      return 0;
  return 1;
}

EXTERNC void print(struct string *s) {
  int i;
  unsigned char *p = s->chars;
  for (i = 0; i < s->length; i++, p++)
    putchar(*p);
}

EXTERNC void printi(int k) { printf("%d", k); }

EXTERNC void flush() { fflush(stdout); }

struct string consts[256];
struct string empty = {0, ""};

int main() {
  int i;
  for (i = 0; i < 256; i++) {
    consts[i].length = 1;
    consts[i].chars[0] = i;
  }
  // Change it to your own implementation after implement heap and delete the
  // comment! tiger_heap = new gc::TigerHeap();
  tiger_heap = new gc::DerivedHeap();
  tiger_heap->Initialize(TIGER_HEAP_SIZE);
  auto ret_val = tigermain(0 /* static link */);
  tiger_heap->FreeHeap();
  return ret_val;
}

EXTERNC int ord(struct string *s) {
  if (s->length == 0)
    return -1;
  else
    return s->chars[0];
}

EXTERNC struct string *chr(int i) {
  if (i < 0 || i >= 256) {
    printf("chr(%d) out of range\n", i);
    exit(1);
  }
  return consts + i;
}

EXTERNC int size(struct string *s) { return s->length; }

EXTERNC struct string *substring(struct string *s, int first, int n) {
  if (first < 0 || first + n > s->length) {
    printf("substring([%d],%d,%d) out of range\n", s->length, first, n);
    exit(1);
  }
  if (n == 1)
    return consts + s->chars[first];
  {
    struct string *t = (struct string *)malloc(sizeof(int) + n);
    int i;
    t->length = n;
    for (i = 0; i < n; i++)
      t->chars[i] = s->chars[first + i];
    return t;
  }
}

EXTERNC struct string *concat(struct string *a, struct string *b) {
  if (a->length == 0)
    return b;
  else if (b->length == 0)
    return a;
  else {
    int i, n = a->length + b->length;
    struct string *t = (struct string *)malloc(sizeof(int) + n);
    t->length = n;
    for (i = 0; i < a->length; i++)
      t->chars[i] = a->chars[i];
    for (i = 0; i < b->length; i++)
      t->chars[i + a->length] = b->chars[i];
    return t;
  }
}

// int not(int i) { return !i; }

#undef getchar

EXTERNC struct string *__wrap_getchar() {
  int i = getc(stdin);
  if (i == EOF)
    return &empty;
  else
    return consts + i;
}
