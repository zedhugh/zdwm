#include "core/policy.h"

#include "base/macros.h"
#include "core/state.h"

layer_type_t policy_adjust_window_layer_from_transient(
  const state_t *state, window_id_t transient_for, layer_type_t base_layer) {
  const window_t *window = state_window_get(state, transient_for);
  return MAX(window->layer, base_layer);
}
