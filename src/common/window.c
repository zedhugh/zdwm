#include "common/window.h"

#include "base/array.h"
#include "base/memory.h"
#include "interface/types.h"

window_layer_type_t window_classify_layer(const window_layer_props_t *props) {
  for (size_t i = 0; i < props->type_count; ++i) {
    window_type_t type = props->types[i];
    if (type == ZDWM_WINDOW_TYPE_NOTIFICATION) return ZDWM_WINDOW_LAYER_OVERLAY;
  }

  for (size_t i = 0; i < props->state_count; ++i) {
    window_state_t state = props->states[i];
    if (state == ZDWM_WINDOW_STATE_ABOVE) return ZDWM_WINDOW_LAYER_TOP;
  }

  return ZDWM_WINDOW_LAYER_NORMAL;
}

void window_layer_props_cleanup(window_layer_props_t *props) {
  if (!props) return;

  p_delete(&props->types);
  props->type_count = 0;
  p_delete(&props->states);
  props->state_count = 0;
}

void window_metadata_cleanup(window_metadata_t *metadata) {
  if (!metadata) return;

  p_delete(&metadata->title);
  p_delete(&metadata->app_id);
  p_delete(&metadata->role);
  p_delete(&metadata->class_name);
  p_delete(&metadata->instance_name);
}

void window_list_push(window_list_t *window_list, zdwm_window_id_t window_id) {
  zdwm_window_id_t *window =
    array_push(window_list->windows, window_list->count, window_list->capacity);
  *window = window_id;
}

void window_list_reset(window_list_t *window_list) {
  p_clear(window_list->windows, window_list->capacity);
  window_list->count = 0;
}

void window_list_cleanup(window_list_t *window_list) {
  p_delete(&window_list->windows);
  window_list->count = 0;
  window_list->capacity = 0;
}
