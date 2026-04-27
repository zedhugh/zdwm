#include "core/policy.h"

#include <stddef.h>
#include <zdwm/types.h>

#include "base/macros.h"
#include "core/binding.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

static void route_key_press(
  binding_table_t *binding_table,
  const zdwm_action_ctx_t *action_ctx,
  const key_press_event_t *e
) {
  size_t count  = 0;
  auto bindings = binding_table_get_current_bindings(binding_table, &count);
  if (!bindings) return;

  for (size_t i = 0; i < count; ++i) {
    auto binding = &bindings[i];
    if (binding->modifiers == e->modifiers && binding->keysym == e->keysym) {
      binding->fn(action_ctx, &binding->arg);
    }
  }
}

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

  command_t manage_window_cmd = {
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
    manage_window_command_t *data = &manage_window_cmd.as.manage_window;
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

  command_buffer_push(out, &manage_window_cmd);
  if (have_rule_match && action.switch_to_workspace) {
    workspace_id_t workspace_id  = manage_window_cmd.as.manage_window.workspace;
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

static void route_window_remove(
  state_t *state,
  const window_remove_event_t *e,
  command_buffer_t *out
) {
  const window_t *window = state_window_get(state, e->window);
  if (!window) return;

  switch (e->reason) {
  case ZDWM_WINDOW_REMOVE_WITHDRAWN:
    command_t withdrawn_window_cmd = {
      .type               = ZDWM_COMMAND_WITHDRAW_WINDOW,
      .as.withdraw.window = e->window,
    };
    command_buffer_push(out, &withdrawn_window_cmd);
    break;
  case ZDWM_WINDOW_REMOVE_DESTROY:
    command_t unmanage_window_cmd = {
      .type               = ZDWM_COMMAND_UNMANAGE_WINDOW,
      .as.unmanage.window = e->window,
    };
    command_buffer_push(out, &unmanage_window_cmd);
    break;
  }
}

void policy_route_event(
  const policy_context_t *ctx,
  const event_t *event,
  command_buffer_t *out
) {
  auto state = ctx->state;
  switch (event->type) {
  case ZDWM_EVENT_KEY_PRESS:
    route_key_press(ctx->bind_table, &ctx->action_ctx, &event->as.key_press);
    break;
  case ZDWM_EVENT_WINDOW_MAP_REQUEST:
    route_map_request(state, ctx->rules, &event->as.window_map_request, out);
    break;
  case ZDWM_EVENT_WINDOW_REMOVE:
    route_window_remove(state, &event->as.window_remove, out);
    break;
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
        .type          = ZDWM_EFFECT_MAP_WINDOW,
        .as.map.window = window->id,
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

static void unmanage_window(state_t *state, window_id_t window, plan_t *plan) {
  const window_t *win = state_window_get(state, window);
  if (!win) return;

  auto workspace_id = win->workspace_id;
  auto workspace    = state_workspace_get(state, workspace_id);
  auto output       = state_output_get(state, workspace->output_id);

  auto old_focused_window = workspace->focused_window_id;
  state_window_remove(state, window);
  if (output->current_workspace_id != workspace_id) return;

  if (old_focused_window != workspace->focused_window_id) {
    effect_t focus_effect = {
      .type            = ZDWM_EFFECT_FOCUS_WINDOW,
      .as.focus.window = workspace->focused_window_id
    };
    plan_push_effect(plan, &focus_effect);
  }
}

static void focus_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  auto workspace_id       = win->workspace_id;
  auto workspace          = state_workspace_get(state, workspace_id);
  auto old_focused_window = workspace->focused_window_id;

  state_workspace_set_focused_window(state, workspace_id, window);
  if (old_focused_window == workspace->focused_window_id) return;

  effect_t focus_effect = {
    .type            = ZDWM_EFFECT_FOCUS_WINDOW,
    .as.focus.window = workspace->focused_window_id,
  };
  plan_push_effect(plan, &focus_effect);
}

static void kill_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  effect_t kill_effect = {
    .type           = ZDWM_EFFECT_KILL_WINDOW,
    .as.kill.window = window,
  };
  plan_push_effect(plan, &kill_effect);
}

static void withdraw_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  effect_t withdraw_window_effect = {
    .type               = ZDWM_EFFECT_WITHDRAW_WINDOW,
    .as.withdraw.window = window,
  };
  plan_push_effect(plan, &withdraw_window_effect);
}

static void fullscreen_window(
  state_t *state,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_FULLSCREEN) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_FULLSCREEN)) {
    return;
  }

  effect_t fullscreen_window_effect = {
    .type          = ZDWM_EFFECT_FULLSCREEN_WINDOW,
    .as.fullscreen = {.window = window_id, .value = value},
  };
  plan_push_effect(plan, &fullscreen_window_effect);

  if (value) {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_FULLSCREEN);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
    }
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
    }
  }

  plan->need_relayout = true;
}

static void maximize_window(
  state_t *state,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_MAXIMIZED) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_MAXIMIZED)) {
    return;
  }

  effect_t maximize_window_effect = {
    .type        = ZDWM_EFFECT_MAXIMIZE_WINDOW,
    .as.maximize = {.window = window_id, .value = value},
  };
  plan_push_effect(plan, &maximize_window_effect);

  if (value) {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_MAXIMIZED);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
    }
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
    }
  }

  plan->need_relayout = true;
}

static void minimize_window(
  state_t *state,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_MINIMIZED) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_MINIMIZED)) {
    return;
  }

  effect_t minimize_window_effect = {
    .type        = ZDWM_EFFECT_MINIMIZE_WINDOW,
    .as.minimize = {.window = window_id, .value = value},
  };
  plan_push_effect(plan, &minimize_window_effect);

  if (value) {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_MINIMIZED);

    effect_t unmap_effect = {
      .type            = ZDWM_EFFECT_UNMAP_WINDOW,
      .as.unmap.window = window_id,
    };
    plan_push_effect(plan, &unmap_effect);
    /* TODO: 可能需要处理焦点 */
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    state_workspace_set_focused_window(state, window->workspace_id, window_id);
    effect_t map_effect = {
      .type          = ZDWM_EFFECT_MAP_WINDOW,
      .as.map.window = window_id
    };
    effect_t focus_effect = {
      .type            = ZDWM_EFFECT_FOCUS_WINDOW,
      .as.focus.window = window_id,
    };
    plan_push_effect(plan, &map_effect);
    plan_push_effect(plan, &focus_effect);
  }

  plan->need_relayout = true;
}

static void change_window_state(
  state_t *state,
  const window_state_change_command_t *command,
  plan_t *plan
) {
  auto window = state_window_get(state, command->window);
  if (!window) return;

  bool value = false;
  switch (command->action) {
  case ZDWM_WINDOW_STATE_ACTION_ADD:
    value = true;
    break;
  case ZDWM_WINDOW_STATE_ACTION_REMOVE:
    value = false;
    break;
  case ZDWM_WINDOW_STATE_ACTION_TOGGLE:
    switch (command->type) {
    case ZDWM_WINDOW_STATE_REQUEST_FULLSCREEN:
      value = !(window->geometry_mode == ZDWM_GEOMETRY_FULLSCREEN);
      break;
    case ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED:
      value = !(window->geometry_mode == ZDWM_GEOMETRY_MAXIMIZED);
      break;
    case ZDWM_WINDOW_STATE_REQUEST_MINIMIZED:
      value = !(window->geometry_mode == ZDWM_GEOMETRY_MINIMIZED);
      break;
    }
  }

  switch (command->type) {
  case ZDWM_WINDOW_STATE_REQUEST_FULLSCREEN:
    fullscreen_window(state, window->id, value, plan);
    break;
  case ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED:
    maximize_window(state, window->id, value, plan);
    break;
  case ZDWM_WINDOW_STATE_REQUEST_MINIMIZED:
    minimize_window(state, window->id, value, plan);
    break;
  }
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
    case ZDWM_COMMAND_UNMANAGE_WINDOW:
      unmanage_window(state, cmd->as.unmanage.window, plan);
      break;
    case ZDWM_COMMAND_FOCUS_WINDOW:
      focus_window(state, cmd->as.focus.window, plan);
      break;
    case ZDWM_COMMAND_KILL_WINDOW:
      kill_window(state, cmd->as.kill.window, plan);
      break;
    case ZDWM_COMMAND_WITHDRAW_WINDOW:
      withdraw_window(state, cmd->as.withdraw.window, plan);
      break;
    case ZDWM_COMMAND_CHANGE_WINDOW_STATE:
      change_window_state(state, &cmd->as.state_change, plan);
      break;
    case ZDWM_COMMAND_SWITCH_WORKSPACE:
      switch_workspace(state, &cmd->as.switch_workspace, plan);
      break;
    }
  }
}
