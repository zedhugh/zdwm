#include "core/policy.h"

#include <stddef.h>
#include <zdwm/types.h>

#include "base/macros.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

static workspace_id_t derive_window_workspace(const state_t *state) {
  return state->outputs[state->current_output_index].current_workspace_id;
}

static void route_map_request(
  const state_t *state,
  const rules_t *rules,
  const window_map_request_event_t *e,
  command_buffer_t *out
) {
  if (state_window_get(state, e->window)) return;

  window_layer_type_t layer_type = window_classify_layer(&e->props);
  if (e->transient_for != ZDWM_WINDOW_ID_INVALID) {
    const window_t *window = state_window_get(state, e->transient_for);
    if (window) layer_type = MAX(window->layer, layer_type);
  }

  command_t cmd = {
    .type             = ZDWM_COMMAND_MANAGE_WINDOW,
    .as.manage_window = {
      .workspace = derive_window_workspace(state),
      .info      = {
             .id            = e->window,
             .transient_for = e->transient_for,
             .frame_rect    = e->rect,

             .title         = e->metadata.title,
             .app_id        = e->metadata.app_id,
             .role          = e->metadata.role,
             .class_name    = e->metadata.class_name,
             .instance_name = e->metadata.instance_name,

             .geometry_mode = e->geometry_mode,
             .layer_type    = layer_type,
             .urgent        = e->urgent,
             .fixed_size    = e->fixed_size,
             .skip_taskbar  = e->skip_taskbar,
      },
    },
  };

  rule_action_t action = {.workspace = ZDWM_WORKSPACE_ID_INVALID};
  bool have_rule_match = rules_resolve(rules, &e->metadata, &action);
  if (have_rule_match) {
    manage_window_command_t *data = &cmd.as.manage_window;
    if (action.workspace != ZDWM_WORKSPACE_ID_INVALID) {
      data->workspace = action.workspace;
    }
    if (action.floating) {
      data->floating = true;
    }

    if (action.fullscreen) {
      data->info.geometry_mode = ZDWM_GEOMETRY_FULLSCREEN;
    } else if (action.maximize) {
      data->info.geometry_mode = ZDWM_GEOMETRY_MAXIMIZED;
    }
  }

  command_buffer_push(out, &cmd);
  if (have_rule_match && action.switch_to_workspace) {
    workspace_id_t workspace_id  = cmd.as.manage_window.workspace;
    const workspace_t *workspace = state_workspace_get(state, workspace_id);

    if (workspace) {
      command_t switch_workspace_cmd = {
        .type                = ZDWM_COMMAND_SWITCH_WORKSPACE,
        .as.switch_workspace = {
          .output    = workspace->output_id,
          .workspace = workspace_id,
        },
      };

      command_buffer_push(out, &switch_workspace_cmd);
    }
  }
}

void policy_route_event(
  const policy_context_t *ctx,
  const event_t *event,
  command_buffer_t *out
) {
  auto state = ctx->state;
  switch (event->type) {
  case ZDWM_EVENT_WINDOW_MAP_REQUEST: {
    auto rules = ctx->rules;
    route_map_request(state, rules, &event->as.window_map_request, out);
    break;
  }
  default:
  }
}

static void manage_window(
  const policy_context_t *ctx,
  const manage_window_command_t *command,
  plan_t *plan
) {
  auto state     = ctx->state;
  auto window    = state_window_add(state, &command->info);
  auto window_id = window->id;
  state_window_set_workspace(state, window_id, command->workspace);
  state_workspace_set_focused_window(state, command->workspace, window_id);
  state_window_set_floating(state, window_id, command->floating);

  if (!state_workspace_show(state, command->workspace)) return;

  effect_t map_effect = {
    .type          = ZDWM_EFFECT_MAP_WINDOW,
    .as.map.window = window_id,
  };
  effect_t focus_effect = {
    .type            = ZDWM_EFFECT_FOCUS_WINDOW,
    .as.focus.window = window_id,
  };
  plan_push_effect(plan, &map_effect);
  plan_push_effect(plan, &focus_effect);

  if (window_need_layout(window)) {
    plan->need_relayout = true;
  }
}

static void add_switch_workspace_effects(
  const state_t *state,
  workspace_id_t old_workspace,
  workspace_id_t new_workspace,
  plan_t *plan
) {
  for (size_t i = 0; i < state->window_count; ++i) {
    window_t *window = &state->windows[i];
    if (window->sticky) continue;

    if (window->workspace_id == old_workspace) {
      effect_t unmap_effect = {
        .type            = ZDWM_EFFECT_UNMAP_WINDOW,
        .as.unmap.window = window->id,
      };
      plan_push_effect(plan, &unmap_effect);
    } else if (window->workspace_id == new_workspace) {
      effect_t map_effect = {
        .type            = ZDWM_EFFECT_MAP_WINDOW,
        .as.unmap.window = window->id,
      };
      plan_push_effect(plan, &map_effect);
    }
  }

  const workspace_t *workspace = state_workspace_get(state, new_workspace);
  if (!workspace) return;

  effect_t focus_effect = {
    .type            = ZDWM_EFFECT_FOCUS_WINDOW,
    .as.focus.window = workspace->focused_window_id
  };
  plan_push_effect(plan, &focus_effect);
}

static void switch_workspace(
  state_t *state,
  const switch_workspace_command_t *command,
  plan_t *plan
) {
  auto output_id               = command->output;
  auto workspace_id            = command->workspace;
  workspace_id_t old_workspace = ZDWM_WORKSPACE_ID_INVALID;

  if (state_output_set_current_workspace(
        state,
        output_id,
        workspace_id,
        &old_workspace
      )) {
    plan->need_relayout = true;
    add_switch_workspace_effects(state, old_workspace, workspace_id, plan);
  }

  state_set_current_output(state, output_id);
}

void policy_apply_command(
  const policy_context_t *ctx,
  const command_buffer_t *command_buffer,
  plan_t *plan
) {
  auto state = ctx->state;
  for (size_t i = 0; i < command_buffer->count; ++i) {
    auto cmd = &command_buffer->items[i];
    switch (cmd->type) {
    case ZDWM_COMMAND_MANAGE_WINDOW:
      manage_window(ctx, &cmd->as.manage_window, plan);
      break;
    case ZDWM_COMMAND_SWITCH_WORKSPACE:
      switch_workspace(state, &cmd->as.switch_workspace, plan);
      break;
    default:
    }
  }
}
