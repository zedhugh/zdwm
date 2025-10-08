#pragma once

#include <assert.h>
#include <string.h>

#include "utils.h"

typedef struct buffer_t {
  char *s;
  int len, size;
  unsigned alloced : 1;
  unsigned offs : 31;
} buffer_t;

extern char buffer_slop[1];

#define BUFFER_INIT (buffer_t){.s = buffer_slop, .size = 1}

/**
 * Initialize a buffer.
 * @param buf A buffer pointer.
 * @return The same buffer pointer.
 */
static inline buffer_t *buffer_init(buffer_t *buf) {
  *buf = BUFFER_INIT;
  return buf;
}

void buffer_ensure(buffer_t *buf, int len);

/**
 * Add data in the buffer.
 * @param buf Buffer where to add.
 * @param pos Position where to add.
 * @param len Length.
 * @param data Data to add.
 * @param dlen Data length.
 */
static inline void buffer_splice(buffer_t *buf, int pos, int len,
                                 const void *data, int dlen) {
  assert(pos >= 0 && len >= 0 && dlen >= 0);

  if (unlikely(pos > buf->len)) pos = buf->len;
  if (unlikely(len > buf->len - pos)) len = buf->len - pos;
  if (pos == 0 && len + buf->offs >= dlen) {
    buf->offs += len - dlen;
    buf->s += len - dlen;
    buf->size -= len - dlen;
    buf->len -= len - dlen;
  } else if (len != dlen) {
    buffer_ensure(buf, buf->len + dlen - len);
    memmove(buf->s + pos + dlen, buf->s + pos + len, buf->len - pos - len);
    buf->len += dlen - len;
    buf->s[buf->len] = '\0';
  }
  memcpy(buf->s + pos, data, dlen);
}

/**
 * Add data at the end of buffer.
 * @param buf Buffer where to add.
 * @param data Data to add.
 * @param len Data length.
 */
static inline void buffer_add(buffer_t *buf, const void *data, int len) {
  buffer_splice(buf, buf->len, 0, data, len);
}

#define buffer_addsl(buf, data) buffer_add(buf, data, sizeof(data) - 1);

/**
 * Add a string to the and of a buffer.
 * @param buf The buffer where to add.
 * @param s The string to add.
 */
static inline void buffer_adds(buffer_t *buf, const char *s) {
  buffer_splice(buf, buf->len, 0, s, a_strlen(s));
}
