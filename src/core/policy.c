#include "core/policy.h"

#include <stddef.h>
#include <stdint.h>
#include <zdwm/types.h>

#include "base/macros.h"
#include "base/window_list.h"
#include "core/binding.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/layout.h"
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

static void
route_pointer_enter(state_t *state, window_id_t window, command_buffer_t *out) {
  command_t focus_command = {
    .type            = ZDWM_COMMAND_FOCUS_WINDOW,
    .as.focus.window = window,
  };
  command_buffer_push(out, &focus_command);
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

static void route_configure_request(
  state_t *state,
  const configure_data_t *data,
  const layout_registry_t *layouts,
  command_buffer_t *out
) {
  auto window = state_window_get(state, data->window);
  if (!window) {
    command_t configure_cmd = {
      .type         = ZDWM_COMMAND_CONFIGURE_WINDOW,
      .as.configure = *data
    };
    command_buffer_push(out, &configure_cmd);
    return;
  }

  if (!state_workspace_show(state, window->workspace_id)) return;
  if (window_need_layout(window)) return;
  auto workspace = state_workspace_get(state, window->workspace_id);
  if (layout_get(layouts, workspace->layout_id)) return;

  command_t configure_rect_cmd = {
    .type         = ZDWM_COMMAND_CONFIGURE_WINDOW,
    .as.configure = *data,
  };
  configure_rect_cmd.as.configure.changed_fields &=
    ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y |
    ZDWM_CONFIGURE_FIELD_WIDTH | ZDWM_CONFIGURE_FIELD_HEIGHT;
  command_buffer_push(out, &configure_rect_cmd);
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
  case ZDWM_EVENT_POINTER_ENTER:
    route_pointer_enter(state, event->as.pointer_enter.window, out);
  case ZDWM_EVENT_WINDOW_MAP_REQUEST:
    route_map_request(state, ctx->rules, &event->as.window_map_request, out);
    break;
  case ZDWM_EVENT_WINDOW_REMOVE:
    route_window_remove(state, &event->as.window_remove, out);
    break;
  case ZDWM_EVENT_CONFIGURE_REQUEST: {
    auto data = &event->as.configure_request;
    route_configure_request(state, data, ctx->layouts, out);
  } break;
  default:
  }
}

static void adjust_layout_windows_border_width(
  state_t *state,
  uint32_t border_width,
  workspace_id_t workspace_id
) {
  static window_list_t list = {0};
  state_get_windows_need_layout_in_workspace(state, workspace_id, &list);
  if (list.count <= 1) border_width = 0;
  for (size_t i = 0; i < list.count; ++i) {
    auto window_id = list.windows[i];
    state_window_set_border_width(state, window_id, border_width);
  }
  window_list_reset(&list);
}

static void set_foucs_window(
  const policy_context_t *ctx,
  workspace_id_t workspace_id,
  window_id_t window_id,
  plan_t *plan
) {
  auto state  = ctx->state;
  auto border = ctx->border;

  auto workspace = state_workspace_get(state, workspace_id);
  if (!workspace) return;

  auto old_focused_window_id = workspace->focused_window_id;
  if (window_id == old_focused_window_id) return;

  state_workspace_set_focused_window(state, workspace_id, window_id);
  plan_push_focus_effect(plan, window_id);

  if (state_window_get(state, window_id)) {
    auto color = &border->focused_color;
    plan_push_change_border_color_effect(plan, window_id, color);
  }
  if (state_window_get(state, old_focused_window_id)) {
    auto color = &border->normal_color;
    plan_push_change_border_color_effect(plan, old_focused_window_id, color);
  }
}

static void manage_window(
  const policy_context_t *ctx,
  const manage_window_command_t *command,
  plan_t *plan
) {
  auto state        = ctx->state;
  auto window       = state_window_add(state, &command->info);
  auto window_id    = window->id;
  auto workspace_id = command->workspace;
  state_window_set_workspace(state, window_id, workspace_id);
  state_window_set_floating(state, window_id, command->floating);
  set_foucs_window(ctx, workspace_id, window_id, plan);

  auto need_layout = window_need_layout(window);
  if (need_layout) {
    adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
  } else {
    state_window_set_border_width(state, window_id, ctx->border->width);
  }

  if (!state_workspace_show(state, command->workspace)) return;

  plan_push_map_effect(plan, window_id);
  plan_push_focus_effect(plan, window_id);

  if (window_need_layout(window)) {
    plan->need_relayout = true;

    return;
  }

  effect_t configure_effect = {
    .type         = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window = window_id,
      .x      = window->frame_rect.x,
      .y      = window->frame_rect.y,
      .width  = window->frame_rect.width - 2 * (int32_t)window->border_width,
      .height = window->frame_rect.height - 2 * (int32_t)window->border_width,
      .border_width   = window->border_width,
      .changed_fields = ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y |
                        ZDWM_CONFIGURE_FIELD_WIDTH |
                        ZDWM_CONFIGURE_FIELD_HEIGHT |
                        ZDWM_CONFIGURE_FIELD_BORDER_WIDTH,
    }
  };
  plan_push_effect(plan, &configure_effect);
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
      plan_push_unmap_effect(plan, window->id);
    } else if (window->workspace_id == new_workspace) {
      plan_push_map_effect(plan, window->id);
    }
  }

  const workspace_t *workspace = state_workspace_get(state, new_workspace);
  if (!workspace) return;

  plan_push_focus_effect(plan, workspace->focused_window_id);
}

static void
unmanage_window(const policy_context_t *ctx, window_id_t window, plan_t *plan) {
  auto state          = ctx->state;
  const window_t *win = state_window_get(state, window);
  if (!win) return;

  auto workspace_id = win->workspace_id;
  auto workspace    = state_workspace_get(state, workspace_id);
  auto output       = state_output_get(state, workspace->output_id);

  auto old_focused_window = workspace->focused_window_id;
  state_window_remove(state, window);

  if (window_need_layout(win)) {
    adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
    plan->need_relayout = true;
  }

  if (output->current_workspace_id != workspace_id) return;

  if (old_focused_window != workspace->focused_window_id) {
    plan_push_focus_effect(plan, workspace->focused_window_id);
  }
}

static void
focus_window(const policy_context_t *ctx, window_id_t window, plan_t *plan) {
  auto state = ctx->state;
  auto win   = state_window_get(state, window);
  if (!win) return;

  auto workspace_id       = win->workspace_id;
  auto workspace          = state_workspace_get(state, workspace_id);
  auto old_focused_window = workspace->focused_window_id;

  set_foucs_window(ctx, workspace_id, window, plan);
  if (old_focused_window == workspace->focused_window_id) return;

  plan_push_focus_effect(plan, workspace->focused_window_id);
}

static void kill_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  plan_push_kill_effect(plan, window);
}

static void withdraw_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  plan_push_withdraw_effect(plan, window);
}

static void
configure_window(state_t *state, const configure_data_t *data, plan_t *plan) {
  auto window = state_window_get(state, data->window);
  if (!window) {
    effect_t configure_effect = {
      .type         = ZDWM_EFFECT_CONFIGURE_WINDOW,
      .as.configure = *data,
    };
    plan_push_effect(plan, &configure_effect);
    return;
  }

  auto rect = window->frame_rect;
#define PICK_FIELD(FIELD_MASK, RECT_FIELD, DATA_FIELD) \
  if (data->changed_fields & (FIELD_MASK)) rect.RECT_FIELD = data->DATA_FIELD

  PICK_FIELD(ZDWM_CONFIGURE_FIELD_X, x, x);
  PICK_FIELD(ZDWM_CONFIGURE_FIELD_Y, y, y);
  PICK_FIELD(ZDWM_CONFIGURE_FIELD_WIDTH, width, width);
  PICK_FIELD(ZDWM_CONFIGURE_FIELD_HEIGHT, height, height);

#undef PICK_FIELD

  auto need_move   = window_need_move(window, rect.x, rect.y);
  auto need_resize = window_need_resize(window, rect.width, rect.height);
  if (!need_move && !need_resize) return;

  state_window_set_frame_rect(state, window->id, rect);

  effect_t configure_effect = {
    .type         = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window = window->id,
      .x      = rect.x,
      .y      = rect.y,
      .width  = rect.width,
      .height = rect.height,
    }
  };
  if (need_move) {
    configure_effect.as.configure.changed_fields |=
      ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y;
  }
  if (need_resize) {
    configure_effect.as.configure.changed_fields |=
      ZDWM_CONFIGURE_FIELD_WIDTH | ZDWM_CONFIGURE_FIELD_HEIGHT;
  }
  plan_push_effect(plan, &configure_effect);
}

static void fullscreen_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto state  = ctx->state;
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_FULLSCREEN) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_FULLSCREEN)) {
    return;
  }

  state_window_set_border_width(state, window_id, 0);
  plan_push_fullscreen_effect(plan, window_id, value);

  if (value) {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_FULLSCREEN);
    state_window_set_border_width(state, window_id, 0);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
    }
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    state_window_set_border_width(state, window_id, ctx->border->width);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
    }
  }

  auto workspace_id = window->workspace_id;
  adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
  plan->need_relayout = true;
}

static void maximize_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto state = ctx->state;

  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_MAXIMIZED) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_MAXIMIZED)) {
    return;
  }

  state_window_set_border_width(state, window_id, 0);
  plan_push_maximize_effect(plan, window_id, value);

  if (value) {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_MAXIMIZED);
    state_window_set_border_width(state, window_id, 0);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
    }
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    state_window_set_border_width(state, window_id, ctx->border->width);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
    }
  }

  auto workspace_id = window->workspace_id;
  adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
  plan->need_relayout = true;
}

static void minimize_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto state  = ctx->state;
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if ((value && window->geometry_mode == ZDWM_GEOMETRY_MINIMIZED) ||
      (!value && window->geometry_mode != ZDWM_GEOMETRY_MINIMIZED)) {
    return;
  }

  plan_push_minimize_effect(plan, window_id, value);

  if (value) {
    plan_push_unmap_effect(plan, window_id);

    auto workspace = state_workspace_get(state, window->workspace_id);
    auto old_focused_window_id = workspace->focused_window_id;
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_MINIMIZED);
    auto new_focused_window_id = workspace->focused_window_id;
    if (old_focused_window_id != new_focused_window_id) {
      auto color = &ctx->border->normal_color;
      plan_push_change_border_color_effect(plan, old_focused_window_id, color);

      color = &ctx->border->focused_color;
      plan_push_change_border_color_effect(plan, new_focused_window_id, color);
    }
  } else {
    state_window_set_geometry_mode(state, window_id, ZDWM_GEOMETRY_NORMAL);
    plan_push_map_effect(plan, window_id);
    set_foucs_window(ctx, window->workspace_id, window_id, plan);
  }

  auto workspace_id = window->workspace_id;
  adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
  plan->need_relayout = true;
}

static void change_window_state(
  const policy_context_t *ctx,
  const window_state_change_command_t *command,
  plan_t *plan
) {
  auto state = ctx->state;

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
    fullscreen_window(ctx, window->id, value, plan);
    break;
  case ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED:
    maximize_window(ctx, window->id, value, plan);
    break;
  case ZDWM_WINDOW_STATE_REQUEST_MINIMIZED:
    minimize_window(ctx, window->id, value, plan);
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
      unmanage_window(ctx, cmd->as.unmanage.window, plan);
      break;
    case ZDWM_COMMAND_FOCUS_WINDOW:
      focus_window(ctx, cmd->as.focus.window, plan);
      break;
    case ZDWM_COMMAND_KILL_WINDOW:
      kill_window(state, cmd->as.kill.window, plan);
      break;
    case ZDWM_COMMAND_WITHDRAW_WINDOW:
      withdraw_window(state, cmd->as.withdraw.window, plan);
      break;
    case ZDWM_COMMAND_CONFIGURE_WINDOW:
      configure_window(state, &cmd->as.configure, plan);
      break;
    case ZDWM_COMMAND_CHANGE_WINDOW_STATE:
      change_window_state(ctx, &cmd->as.state_change, plan);
      break;
    case ZDWM_COMMAND_SWITCH_WORKSPACE:
      switch_workspace(state, &cmd->as.switch_workspace, plan);
      break;
    }
  }
}
