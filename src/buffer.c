#include "buffer.h"

#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "utils.h"

char buffer_slop[1];

void buffer_ensure(buffer_t *buf, int newlen) {
  if (newlen < 0) exit(EX_SOFTWARE);

  if (newlen < buf->size) return;

  if (newlen < buf->offs + buf->size && buf->offs > buf->size / 4) {
    /* Data fits in the current area, shift it left */
    memmove(buf->s - buf->offs, buf->s, buf->len + 1);
    buf->s -= buf->offs;
    buf->size += buf->offs;
    buf->offs = 0;
    return;
  }

  buf->size = p_alloc_nr(buf->size + buf->offs);
  if (buf->size < newlen + 1) buf->size = newlen + 1;
  if (buf->alloced && !buf->offs)
    p_realloc(&buf->s, buf->size);
  else {
    char *new_area = xmalloc(buf->size);
    memcpy(new_area, buf->s, buf->len + 1);
    if (buf->alloced) free(buf->s - buf->offs);
    buf->alloced = true;
    buf->s = new_area;
    buf->offs = 0;
  }
}
