#pragma once

#include "interface/types.h"

window_layer_type_t window_classify_layer(const window_layer_props_t *props);
void window_layer_props_cleanup(window_layer_props_t *props);
void window_metadata_cleanup(window_metadata_t *metadata);

typedef struct window_list_t {
  zdwm_window_id_t *windows;
  size_t count;
  size_t capacity;
} window_list_t;

void window_list_push(window_list_t *window_list, zdwm_window_id_t window_id);
void window_list_reset(window_list_t *window_list);
void window_list_cleanup(window_list_t *window_list);
