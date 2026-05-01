#include "base/window_list.h"

#include <zdwm/types.h>

#include "base/array.h"
#include "base/memory.h"

void window_list_push(
  window_list_t *window_list,
  zdwm_window_id_t window_id
) {
  zdwm_window_id_t *window =
    array_push(window_list->windows, window_list->count, window_list->capacity);
  *window = window_id;
}

void window_list_reset(window_list_t *window_list) {
  p_clear(window_list->windows, window_list->capacity);
  window_list->count = 0;
}

void window_list_cleanup(window_list_t *window_list) {
  p_delete(window_list->windows);
  window_list->count    = 0;
  window_list->capacity = 0;
}
