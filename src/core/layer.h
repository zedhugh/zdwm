#pragma once

#include <stddef.h>

#include "core/types.h"
#include "core/window.h"

/* 层级从低到高 */
typedef enum layer_type_t {
  ZDWM_WINDOW_LAYER_DESKTOP = 0,
  ZDWM_WINDOW_LAYER_NORMAL,
  ZDWM_WINDOW_LAYER_TOP,
  ZDWM_WINDOW_LAYER_OVERLAY,
  ZDWM_WINDOW_LAYER_COUNT,
} layer_type_t;

typedef struct layer_stack_t {
  window_id_t *order;
  size_t count;
  size_t capacity;
} layer_stack_t;

void layer_stack_init(layer_stack_t *layer);
void layer_stack_reset(layer_stack_t *layer);
void layer_stack_append(layer_stack_t *layer, window_id_t window);
void layer_stack_remove(layer_stack_t *layer, window_id_t window);
void layer_stack_raise(layer_stack_t *layer, window_id_t window);
void layer_stack_lower(layer_stack_t *layer, window_id_t window);

layer_type_t layer_classify(const window_props_t *props);
