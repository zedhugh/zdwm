#pragma once

#include <stdlib.h>
#include <string.h>

#define ssizeof(foo) (ssize_t)sizeof(foo)
#define countof(foo) (ssizeof(foo) / ssizeof(foo[0]))

#define p_alloc_nr(x) (((x) + 16) * 3 / 2)
#define p_new(type, count) ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count) ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count) xrealloc((void *)(pp), sizeof(**(pp)) * (count))

#define p_delete(mem_p)              \
  do {                               \
    void **__ptr = (void **)(mem_p); \
    free(*__ptr);                    \
    *(void **)__ptr = nullptr;       \
  } while (0)

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect((expr), 0)
#else
#define likely(expr) expr
#define unlikely(expr) expr
#endif

static inline void *__attribute__((malloc)) xmalloc(ssize_t size) {
  void *ptr;

  if (size <= 0) return nullptr;

  ptr = calloc(1, size);

  if (!ptr) abort();

  return ptr;
}

static inline void xrealloc(void **ptr, ssize_t newsize) {
  if (newsize <= 0)
    p_delete(ptr);
  else {
    *ptr = realloc(*ptr, newsize);
    if (!*ptr) abort();
  }
}

/**
 * @brief nullptr resistant strlen.
 *
 * Unlike it's libc sibling, a_strlen returns a ssize_t, and supports its
 * argument being nullptr.
 *
 * @param s the string.
 * @return the string length (or 0 if s is nullptr).
 */
static inline ssize_t a_strlen(const char *s) { return s ? strlen(s) : 0; }

#define fatal(format, ...) \
  _fatal(__LINE__, __FUNCTION__, __FILE__, format, ##__VA_ARGS__)
void _fatal(int line, const char *function, const char *file,
            const char *format, ...) __attribute__((noreturn))
__attribute__((format(printf, 4, 5)));

#define warn(format, ...) \
  _fatal(__LINE__, __FUNCTION__, __FILE__, format, ##__VA_ARGS__)
void _warn(int line, const char *function, const char *file, const char *format,
           ...) __attribute__((format(printf, 4, 5)));

const char *current_time_str(void);
