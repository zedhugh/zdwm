#pragma once

#include <stddef.h>

#include "core/command.h"

typedef struct command_buffer_t {
  command_t *items;
  size_t count;
  size_t capacity;
} command_buffer_t;

void command_buffer_reset(command_buffer_t *buffer);
void command_buffer_cleanup(command_buffer_t *buffer);
void command_buffer_push(command_buffer_t *buffer, const command_t *command);
