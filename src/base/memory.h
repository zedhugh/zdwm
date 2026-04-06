#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define p_alloc_nr(x) (((x) + 16) * 3 / 2)
#define p_new(type, count) ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count) ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count) xrealloc((void *)(pp), sizeof(**(pp)) * (count))
#define p_copy(src, count) xmemcopy((src), sizeof(*(src)) * (count))

#define p_delete(mem_p)              \
  do {                               \
    void **__ptr = (void **)(mem_p); \
    free(*__ptr);                    \
    *(void **)__ptr = nullptr;       \
  } while (0)

#define p_take(dst_p, src_p) \
  do {                       \
    p_delete(dst_p);         \
    *(dst_p) = *(src_p);     \
    *(src_p) = nullptr;      \
  } while (0)

static inline char *p_strdup(const char *text) {
  char *r = strdup(text);
  if (!r) abort();
  return r;
}

static inline char *p_strndup(const char *text, size_t n) {
  char *r = strndup(text, n);
  if (!r) abort();
  return r;
}

static inline char *p_strdup_nullable(const char *text) {
  if (!text) return nullptr;
  return p_strdup(text);
}

static inline void *__attribute__((malloc)) xmalloc(ssize_t size) {
  void *ptr;

  if (size <= 0) return nullptr;

  ptr = calloc(1, size);

  if (!ptr) abort();

  return ptr;
}

static inline void *xmemcopy(const void *src, size_t n) {
  if (!n) return nullptr;

  void *mem = xmalloc(n);
  void *ret = memcpy(mem, src, n);
  if (ret != mem) abort();
  return ret;
}

static inline void xrealloc(void **ptr, ssize_t newsize) {
  if (newsize <= 0)
    p_delete(ptr);
  else {
    *ptr = realloc(*ptr, newsize);
    if (!*ptr) abort();
  }
}

/*
 * Unlike strlen(), a_strlen() accepts nullptr and returns 0 in that case.
 */
static inline ssize_t a_strlen(const char *s) { return s ? strlen(s) : 0; }
