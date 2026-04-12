#pragma once

#include "core/command_buffer.h"
#include "core/event.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"

typedef struct policy_context_t {
  const state_t *state;
  const rules_t *rules;
  const event_t *event;
} policy_context_t;

bool policy_route_event(const policy_context_t *ctx, command_buffer_t *out);
bool policy_apply_command(const policy_context_t *ctx,
                          const command_buffer_t *command_buffer, plan_t *plan);
