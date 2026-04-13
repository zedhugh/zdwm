#pragma once

#include "wm_binding.h"
#include "wm_command.h"
#include "wm_event.h"
#include "wm_layout.h"
#include "wm_plan.h"
#include "wm_policy_config.h"
#include "wm_state.h"

typedef struct wm_command_buffer_t {
  wm_command_t *items;
  size_t count;
  size_t capacity;
} wm_command_buffer_t;

void wm_command_buffer_init(wm_command_buffer_t *buffer);
void wm_command_buffer_reset(wm_command_buffer_t *buffer);
void wm_command_buffer_shutdown(wm_command_buffer_t *buffer);

bool wm_command_buffer_push(wm_command_buffer_t *buffer, wm_command_t command);

bool wm_policy_route_event(
  const wm_state_t *state,
  const wm_policy_config_t *policy,
  const wm_key_binding_table_t *keybindings,
  const wm_pointer_binding_table_t *pointer_bindings,
  const wm_event_t *event,
  wm_command_buffer_t *out
);

bool wm_policy_apply_command(
  wm_state_t *state,
  const wm_policy_config_t *policy,
  const wm_layout_registry_t *layouts,
  const wm_command_t *command,
  wm_plan_t *plan
);
