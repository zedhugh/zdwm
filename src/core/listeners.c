#include "core/listeners.h"

#include "base/array.h"

#define ADD_LISTENER(FIELD)                                               \
  auto list       = &listeners->FIELD;                                    \
  auto item       = array_push(list->items, list->count, list->capacity); \
  item->fn        = fn;                                                   \
  item->user_data = user_data

void listeners_add_output_listener(
  listeners_t *listeners,
  zdwm_current_output_id_listener fn,
  void *user_data
) {
  ADD_LISTENER(output_listeners);
}

void listeners_add_initial_workspace_listener(
  listeners_t *listeners,
  zdwm_initial_workspace_list fn,
  void *user_data
) {
  ADD_LISTENER(initial_workspace_listeners);
}

void listeners_add_active_workspace_listener(
  listeners_t *listeners,
  zdwm_workspace_active_updated fn,
  void *user_data
) {
  ADD_LISTENER(active_workspace_listeners);
}

void listeners_add_layout_notify(
  listeners_t *listeners,
  zdwm_layout_notify fn,
  void *user_data
) {
  ADD_LISTENER(layout_listeners);
}

void listeners_add_binding_mode_notify(
  listeners_t *listeners,
  zdwm_binding_mode_notify fn,
  void *user_data
) {
  ADD_LISTENER(binding_mode_listeners);
}

void listeners_add_initial_window_listener(
  listeners_t *listeners,
  zdwm_initial_window_list fn,
  void *user_data
) {
  ADD_LISTENER(initial_window_listeners);
}

void listeners_add_window_added_listener(
  listeners_t *listeners,
  zdwm_window_added fn,
  void *user_data
) {
  ADD_LISTENER(window_added_listeners);
}

void listeners_add_window_updated_listener(
  listeners_t *listeners,
  zdwm_window_updated fn,
  void *user_data
) {
  ADD_LISTENER(window_updated_listeners);
}

void listeners_add_window_removed_listener(
  listeners_t *listeners,
  zdwm_window_removed fn,
  void *user_data
) {
  ADD_LISTENER(window_removed_listeners);
}

#undef ADD_LISTENER
