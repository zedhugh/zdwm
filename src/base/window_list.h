#pragma once

#include <stddef.h>
#include <zdwm/types.h>

typedef struct window_list_t {
  zdwm_window_id_t *windows;
  size_t count;
  size_t capacity;
} window_list_t;

void window_list_push(window_list_t *window_list, zdwm_window_id_t window_id);
void window_list_reset(window_list_t *window_list);
void window_list_cleanup(window_list_t *window_list);
