#include "core/command_buffer.h"

#include "base/array.h"
#include "base/memory.h"
#include "core/command.h"

void command_buffer_reset(command_buffer_t *buffer) {
  if (!buffer->items || !buffer->capacity) return;

  p_clear(buffer->items, buffer->capacity);
  buffer->count = 0;
}

void command_buffer_cleanup(command_buffer_t *buffer) {
  p_delete(&buffer->items);
  buffer->count = 0;
  buffer->capacity = 0;
}

void command_buffer_push(command_buffer_t *buffer, const command_t *command) {
  command_t *cmd = array_push(buffer->items, buffer->count, buffer->capacity);
  *cmd = *command;
}
