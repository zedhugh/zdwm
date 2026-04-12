#include "core/policy.h"

#include <zdwm/types.h>

#include "base/macros.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/layer.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/wm_desc.h"

static workspace_id_t derive_window_workspace(const state_t *state) {
  return state->outputs[state->current_output_index].current_workspace_id;
}

static bool route_map_request(const state_t *state, const rules_t *rules,
                              const window_map_request_event_t *e,
                              command_buffer_t *out) {
  if (state_window_get(state, e->window)) return false;

  layer_type_t layer_type = layer_classify(&e->props);
  if (e->transient_for != ZDWM_WINDOW_ID_INVALID) {
    const window_t *window = state_window_get(state, e->transient_for);
    if (window) layer_type = MAX(window->layer, layer_type);
  }

  command_t cmd = {
    .type = ZDWM_COMMAND_MANAGE_WINDOW,
    .as.manage_window =
      {
        .workspace = derive_window_workspace(state),
        .info =
          {
            .id = e->window,
            .transient_for = e->transient_for,
            .frame_rect = e->rect,

            .title = e->metadata.title,
            .app_id = e->metadata.app_id,
            .role = e->metadata.role,
            .class_name = e->metadata.class_name,
            .instance_name = e->metadata.instance_name,

            .geometry_mode = e->geometry_mode,
            .layer_type = layer_type,
            .urgent = e->urgent,
            .fixed_size = e->fixed_size,
            .skip_taskbar = e->skip_taskbar,
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
    workspace_id_t workspace_id = cmd.as.manage_window.workspace;
    const workspace_t *workspace = state_workspace_get(state, workspace_id);

    if (workspace) {
      command_t switch_workspace_cmd = {
        .type = ZDWM_COMMAND_SWITCH_WORKSPACE,
        .as.switch_workspace =
          {
            .output = workspace->output_id,
            .workspace = workspace_id,
          },
      };

      command_buffer_push(out, &switch_workspace_cmd);
    }
  }

  return true;
}

bool policy_route_event(const policy_context_t *ctx, command_buffer_t *out) {
  switch (ctx->event->type) {
    case ZDWM_EVENT_WINDOW_MAP_REQUEST:
      return route_map_request(ctx->state, ctx->rules,
                               &ctx->event->as.window_map_request, out);
    default:
      return false;
  }
}

bool policy_apply_command(const policy_context_t *ctx,
                          const command_buffer_t *command_buffer,
                          plan_t *plan) {
  /* TODO: */
  return true;
}
