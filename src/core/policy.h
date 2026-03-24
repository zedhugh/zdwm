#pragma once

#include "core/layer.h"
#include "core/state.h"
#include "core/types.h"

layer_type_t policy_adjust_window_layer_from_transient(
  const state_t *state, window_id_t transient_for, layer_type_t base_layer);
