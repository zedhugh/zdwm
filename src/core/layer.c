#include "core/layer.h"

#include <stddef.h>

#include "base/array.h"
#include "base/memory.h"
#include "core/types.h"
#include "core/window.h"

void layer_stack_cleanup(layer_stack_t *layer) {
  p_delete(&layer->order);
  layer->count = 0;
  layer->capacity = 0;
}

void layer_stack_append(layer_stack_t *layer, window_id_t window) {
  window_id_t *win_ptr =
    array_push(layer->order, layer->count, layer->capacity);
  *win_ptr = window;
}

bool layer_stack_remove(layer_stack_t *layer, window_id_t window) {
  bool found = false;
  size_t index = 0;
  for (size_t i = 0; i < layer->count; ++i) {
    if (layer->order[i] != window) continue;

    found = true;
    index = i;
    break;
  }

  if (found) {
    for (size_t i = index; i < layer->count - 1; ++i) {
      layer->order[i] = layer->order[i + 1];
    }
    layer->count--;
  }

  return found;
}

bool layer_stack_raise(layer_stack_t *layer, window_id_t window) {
  bool found = false;
  size_t index = 0;
  for (size_t i = 0; i < layer->count; ++i) {
    if (layer->order[i] != window) continue;

    found = true;
    index = i;
    break;
  }

  if (!found) return false;

  bool need_change = index != layer->count - 1;
  if (!need_change) return false;

  for (size_t i = index; i < layer->count - 1; ++i) {
    layer->order[i] = layer->order[i + 1];
  }
  layer->order[layer->count - 1] = window;

  return true;
}

bool layer_stack_lower(layer_stack_t *layer, window_id_t window) {
  bool found = false;
  size_t index = 0;
  for (size_t i = 0; i < layer->count; ++i) {
    if (layer->order[i] != window) continue;

    found = true;
    index = i;
    break;
  }

  if (!found) return false;

  bool need_change = index != 0;
  if (!need_change) return false;

  for (size_t i = index; i > 0; --i) {
    layer->order[i] = layer->order[i - 1];
  }
  layer->order[0] = window;

  return true;
}

layer_type_t layer_classify(const window_layer_props_t *props) {
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
