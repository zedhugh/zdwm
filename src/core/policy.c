#include "core/policy.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zdwm/action.h>
#include <zdwm/types.h>

#include "base/macros.h"
#include "base/memory.h"
#include "base/process.h"
#include "base/time.h"
#include "base/window_list.h"
#include "core/binding.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/layout.h"
#include "core/listeners.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

static void quit(bool restart, command_buffer_t *command_buffer) {
  command_t quit_command = {
    .type = ZDWM_COMMAND_QUIT,
    .as.quit.will_restart = restart
  };
  command_buffer_push(command_buffer, &quit_command);
}

static const window_t *
get_next_window_by_class(const state_t *state, const char *class_name) {
  auto output = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);

  size_t start_index = 0;
  if (!window_id_invalid(workspace->focused_window_id)) {
    for (size_t i = 0; i < state_window_count(state); ++i) {
      if (state_window_at(state, i)->id == workspace->focused_window_id) {
        start_index = i;
        break;
      }
    }
  }

  for (size_t i = start_index + 1; i < state_window_count(state); ++i) {
    auto window = state_window_at(state, i);
    if (strcmp(window->class_name, class_name) == 0) {
      return window;
    }
  }

  for (size_t i = 0; i < MIN(start_index + 1, state_window_count(state)); ++i) {
    auto window = state_window_at(state, i);
    if (strcmp(window->class_name, class_name) == 0) {
      return window;
    }
  }
  return nullptr;
}

static void add_switch_workspace_command(
  command_buffer_t *command_buffer,
  workspace_id_t workspace
) {
  command_t switch_workspace_cmd = {
    .type = ZDWM_COMMAND_SWITCH_WORKSPACE,
    .as.switch_workspace.workspace = workspace,
  };
  command_buffer_push(command_buffer, &switch_workspace_cmd);
}

static void
add_focus_window_command(command_buffer_t *command_buffer, window_id_t window) {
  command_t focus_command = {
    .type = ZDWM_COMMAND_FOCUS_WINDOW,
    .as.focus.window = window,
  };
  command_buffer_push(command_buffer, &focus_command);
}

static void raise_or_run(
  const state_t *state,
  const zdwm_action_data_raise_or_run_t *data,
  command_buffer_t *command_buffer
) {
  auto window = get_next_window_by_class(state, data->class_name);
  if (!window) {
    spawn(data->command);
    return;
  }

  add_switch_workspace_command(command_buffer, window->workspace_id);
  add_focus_window_command(command_buffer, window->id);
}

static output_id_t get_cycled_output(const state_t *state, int32_t delta) {
  if (state->output_count <= 1) return ZDWM_OUTPUT_ID_INVALID;

  int64_t count = (int64_t)state->output_count;
  int64_t current = (int64_t)state->current_output_index;
  int64_t next = (current + (int64_t)delta) % count;
  if (next < 0) next += count;

  auto output = state_output_at(state, (size_t)next);
  return output->id;
}

static void cycle_current_output(
  const state_t *state,
  int32_t delta,
  command_buffer_t *command_buffer
) {
  auto output_id = get_cycled_output(state, delta);
  if (output_id == ZDWM_OUTPUT_ID_INVALID) return;

  command_t cmd = {
    .type = ZDWM_COMMAND_SET_CURRENT_OUTPUT,
    .as.current_output.output = output_id,
  };
  command_buffer_push(command_buffer, &cmd);
}

static void switch_workspace_same_output_by_index(
  const state_t *state,
  uint32_t index,
  command_buffer_t *command_buffer
) {
  auto output = state_output_at(state, state->current_output_index);
  uint32_t count = 0;
  for (size_t i = 0; i < state_workspace_count(state); ++i) {
    auto workspace = state_workspace_at(state, i);
    if (workspace->output_id != output->id) continue;

    if (count == index) {
      add_switch_workspace_command(command_buffer, workspace->id);
      return;
    }
    count++;
  }
}

static void cycle_layout(
  const state_t *state,
  int32_t delta,
  command_buffer_t *command_buffer
) {
  auto output = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);

  int32_t index = 0;
  bool matched = false;
  auto layout_count = (int32_t)workspace->layout_count;
  if (layout_count <= 1) return;

  for (int32_t i = 0; i < layout_count; ++i) {
    if (workspace->layout_id == workspace->available_layouts[i]) {
      index = i;
      matched = true;
      break;
    }
  }

  if (!matched) return;

  auto new_index = index + delta;
  new_index %= layout_count;
  if (new_index < 0) new_index += layout_count;

  if (new_index == index) return;

  auto next_layout_id = workspace->available_layouts[new_index];
  command_t set_layout_command = {
    .type = ZDWM_COMMAND_SET_LAYOUT,
    .as.layout = {.workspace = workspace->id, .layout = next_layout_id},
  };
  command_buffer_push(command_buffer, &set_layout_command);
}

static void cycle_binding_mode(
  const binding_table_t *table,
  int32_t delta,
  command_buffer_t *command_buffer
) {
  auto next_mode_id = binding_table_cycle_mode(table, delta);

  if (next_mode_id == ZDWM_BINDING_MODE_ID_INVALID) return;

  command_t set_binding_mode_command = {
    .type = ZDWM_COMMAND_SET_BINDING_MODE,
    .as.binding_mode.mode = next_mode_id,
  };
  command_buffer_push(command_buffer, &set_binding_mode_command);
}

static const window_t *get_current_focused_window(const state_t *state) {
  auto output = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);
  return state_window_get(state, workspace->focused_window_id);
}

#define DEFUN_WINDOW_TOGGLE_FN(NAME, TYPE, DATA_FIELD, WINDOW_FIELD)           \
  static void NAME(const state_t *state, command_buffer_t *command_buffer) {   \
    auto window = get_current_focused_window(state);                           \
    if (!window) return;                                                       \
                                                                               \
    command_t command = {                                                      \
      .type = (TYPE),                                                          \
      .as.DATA_FIELD = {.window = window->id, .state = !window->WINDOW_FIELD}, \
    };                                                                         \
    command_buffer_push(command_buffer, &command);                             \
  }

DEFUN_WINDOW_TOGGLE_FN(
  toggle_window_fullscreen,
  ZDWM_COMMAND_WINDOW_SET_FULLSCREEN,
  fullscreen,
  fullscreen
)
DEFUN_WINDOW_TOGGLE_FN(
  toggle_window_maximize,
  ZDWM_COMMAND_WINDOW_SET_MAXIMIZED,
  maximized,
  maximized
)
DEFUN_WINDOW_TOGGLE_FN(
  toggle_window_floating,
  ZDWM_COMMAND_WINDOW_SET_FLOATING,
  floating,
  floating
)
DEFUN_WINDOW_TOGGLE_FN(
  toggle_window_sticky,
  ZDWM_COMMAND_WINDOW_SET_STICKY,
  sticky,
  sticky
)

#undef DEFUN_WINDOW_TOGGLE_FN

static void cycle_focused_window(
  const state_t *state,
  int32_t delta,
  command_buffer_t *command_buffer
) {
  auto output = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);

  size_t count = state_window_count(state);
  if (!count) return;

  size_t indices[count];
  size_t num = 0;
  for (size_t i = 0; i < count; ++i) {
    auto window = state_window_at(state, i);
    if (window->workspace_id == workspace->id) {
      indices[num++] = i;
    }
  }
  if (!num) return;

  size_t current = 0;
  if (!window_id_invalid(workspace->focused_window_id)) {
    for (size_t i = 0; i < num; ++i) {
      auto window = state_window_at(state, indices[i]);
      if (window->id == workspace->focused_window_id) {
        current = i;
        break;
      }
    }
  }

  int64_t next = ((int64_t)current + (int64_t)delta) % (int64_t)num;
  if (next < 0) next += (int64_t)num;

  auto target = state_window_at(state, indices[next]);

  add_focus_window_command(command_buffer, target->id);

  command_t raise_cmd = {
    .type = ZDWM_COMMAND_RAISE_WINDOW,
    .as.raise.window = target->id,
  };
  command_buffer_push(command_buffer, &raise_cmd);
}

static void
kill_window_action(const state_t *state, command_buffer_t *command_buffer) {
  auto window = get_current_focused_window(state);
  if (!window) return;

  command_t kill_window_command = {
    .type = ZDWM_COMMAND_KILL_WINDOW,
    .as.kill.window = window->id,
  };
  command_buffer_push(command_buffer, &kill_window_command);
}

static void cycle_window_output(
  const state_t *state,
  const zdwm_action_data_window_cycle_output_t *data,
  command_buffer_t *command_buffer
) {
  auto window = get_current_focused_window(state);
  if (!window) return;

  auto output_id = get_cycled_output(state, data->delta);
  if (output_id == ZDWM_OUTPUT_ID_INVALID) return;

  if (data->keep_focus) {
    command_t set_current_output_command = {
      .type = ZDWM_COMMAND_SET_CURRENT_OUTPUT,
      .as.current_output.output = output_id,
    };
    command_buffer_push(command_buffer, &set_current_output_command);
  }

  auto output = state_output_get(state, output_id);
  auto workspace = state_workspace_get(state, output->current_workspace_id);
  command_t send_window_to_workspace_command = {
    .type = ZDWM_COMMAND_WINDOW_SEND_TO_WORKSPACE,
    .as.send_to_workspace = {
      .window = window->id,
      .workspace = workspace->id,
    },
  };
  command_buffer_push(command_buffer, &send_window_to_workspace_command);
}

static void send_window_to_workspace_same_output_by_index(
  const state_t *state,
  const zdwm_action_data_window_send_to_workspace_t *data,
  command_buffer_t *command_buffer
) {
  auto target_workspace = state_workspace_at(state, (size_t)data->index);
  if (!target_workspace) return;

  auto output = state_output_at(state, state->current_output_index);
  auto current_workspace =
    state_workspace_get(state, output->current_workspace_id);
  if (current_workspace->id == target_workspace->id) return;

  if (data->switch_workspace) {
    add_switch_workspace_command(command_buffer, target_workspace->id);
  }

  command_t send_window_to_workspace_command = {
    .type = ZDWM_COMMAND_WINDOW_SEND_TO_WORKSPACE,
    .as.send_to_workspace = {
      .window = current_workspace->focused_window_id,
      .workspace = target_workspace->id,
    },
  };
  command_buffer_push(command_buffer, &send_window_to_workspace_command);
}

static void toggle_bar_visibility(
  const policy_bar_t *bar,
  command_buffer_t *command_buffer
) {
  command_t set_bar_visibility_command = {
    .type = ZDWM_COMMAND_SET_BAR_VISIBILITY,
    .as.bar_visibility = {
      .visible = !*bar->visible,
    },
  };
  command_buffer_push(command_buffer, &set_bar_visibility_command);
}

void policy_resolve_action(
  const policy_context_t *ctx,
  const zdwm_action_t *action,
  command_buffer_t *out
) {
  switch (action->type) {
  case ZDWM_ACTION_NONE:
    break;
  case ZDWM_ACTION_SPAWN:
    spawn(action->as.spawn.command);
    break;
  case ZDWM_ACTION_QUIT:
    quit(action->as.quit.restart, out);
    break;
  case ZDWM_ACTION_RAISE_OR_RUN:
    raise_or_run(ctx->state, &action->as.raise_or_run, out);
    break;
  case ZDWM_ACTION_OUTPUT_CYCLE:
    cycle_current_output(ctx->state, action->as.output_cycle.delta, out);
    break;
  case ZDWM_ACTION_WORKSPACE_SWITCH:
    add_switch_workspace_command(out, action->as.switch_workspace.workspace);
    break;
  case ZDWM_ACTION_WORKSPACE_SWITCH_SAME_OUTPUT_BY_INDEX: {
    auto index = action->as.workspace_switch_same_output_by_index.index;
    switch_workspace_same_output_by_index(ctx->state, index, out);
  } break;
  case ZDWM_ACTION_LAYOUT_CYCLE:
    cycle_layout(ctx->state, action->as.layout_cycle.delta, out);
    break;
  case ZDWM_ACTION_BINDING_MODE_CYCLE: {
    auto delta = action->as.binding_mode_cycle.delta;
    cycle_binding_mode(ctx->bind_table, delta, out);
  } break;
  case ZDWM_ACTION_WINDOW_TOGGLE_FULLSCREEN:
    toggle_window_fullscreen(ctx->state, out);
    break;
  case ZDWM_ACTION_WINDOW_TOGGLE_MAXIMIZE:
    toggle_window_maximize(ctx->state, out);
    break;
  case ZDWM_ACTION_WINDOW_TOGGLE_MINIMIZE:
    /* TODO: 恢复需要单独的逻辑，需要考虑是否将最小化和恢复拆成两个 action */
    break;
  case ZDWM_ACTION_WINDOW_TOGGLE_FLOATING:
    toggle_window_floating(ctx->state, out);
    break;
  case ZDWM_ACTION_WINDOW_TOGGLE_STICKY:
    toggle_window_sticky(ctx->state, out);
    break;
  case ZDWM_ACTION_WINDOW_FOCUS_CYCLE:
    cycle_focused_window(ctx->state, action->as.window_focus_cycle.delta, out);
    break;
  case ZDWM_ACTION_WINDOW_KILL:
    kill_window_action(ctx->state, out);
    break;
  case ZDWM_ACTION_WINDOW_SEND_TO_WORKSPACE_SAME_OUTPUT_BY_INDEX:
    send_window_to_workspace_same_output_by_index(
      ctx->state,
      &action->as.window_send_to_workspace_same_output_by_index,
      out
    );
    break;
  case ZDWM_ACTION_WINDOW_CYCLE_OUTPUT:
    cycle_window_output(ctx->state, &action->as.window_cycle_output, out);
    break;
  case ZDWM_ACTION_BAR_TOGGLE_VISIBILITY:
    toggle_bar_visibility(&ctx->bar, out);
    break;
  }
}

static void route_key_press(
  const policy_context_t *ctx,
  const key_press_event_t *e,
  command_buffer_t *out
) {
  auto binding_table = ctx->bind_table;

  size_t count = 0;
  auto bindings = binding_table_get_current_bindings(binding_table, &count);
  if (!bindings) return;

  for (size_t i = 0; i < count; ++i) {
    auto binding = &bindings[i];
    if (binding->modifiers == e->modifiers && binding->keysym == e->keysym) {
      policy_resolve_action(ctx, &binding->action, out);
    }
  }
}

static void route_pointer_press(
  const policy_context_t *ctx,
  const pointer_button_event_t *data,
  command_buffer_t *out
) {
  auto window_id = data->window;
  auto window = state_window_get(ctx->state, window_id);
  if (!window) return;

  /* TODO: 后续改为配置 */
  if (data->button == ZDWM_BUTTON_LEFT && data->modifiers == ZDWM_MOD_4) {
    command_t start_move_cmd = {
      .type = ZDWM_COMMAND_START_MOVE_WINDOW,
      .as.move = {
        .window = window_id,
        .pointer_coordinate = data->root,
      },
    };
    command_buffer_push(out, &start_move_cmd);
  }
  if (data->button == ZDWM_BUTTON_RIGHT && data->modifiers == ZDWM_MOD_4) {
    command_t start_resize_cmd = {
      .type = ZDWM_COMMAND_START_RESIZE_WINDOW,
      .as.resize = {
        .window = window_id,
        .pointer_coordinate = data->root,
      }
    };
    command_buffer_push(out, &start_resize_cmd);
  }
}

static void route_pointer_release(
  const policy_context_t *ctx,
  const pointer_button_event_t *data,
  command_buffer_t *out
) {
  switch (ctx->interaction->mode) {
  case ZDWM_WINDOW_INTERACTION_NONE:
    break;
  case ZDWM_WINDOW_INTERACTION_MOVE: {
    command_t stop_move_cmd = {.type = ZDWM_COMMAND_STOP_MOVE_WINDOW};
    command_buffer_push(out, &stop_move_cmd);
  } break;
  case ZDWM_WINDOW_INTERACTION_RESIZE: {
    command_t stop_resize_cmd = {.type = ZDWM_COMMAND_STOP_RESIZE_WINDOW};
    command_buffer_push(out, &stop_resize_cmd);
  } break;
  }
}

static void route_pointer_motion(
  const policy_context_t *ctx,
  const pointer_motion_event_t *data,
  command_buffer_t *out
) {
  auto interaction = ctx->interaction;
  auto window_id = interaction->window;

  switch (interaction->mode) {
  case ZDWM_WINDOW_INTERACTION_NONE:
    break;
  case ZDWM_WINDOW_INTERACTION_MOVE: {
    auto time = interaction->last_change_time;
    auto now = time_monotonic_ms();
    if (now - time < 1000 / ctx->fps) break;

    auto rect = interaction->origin_rect;
    auto start = interaction->start_coordinate;
    auto end = data->root;

    command_t configure = {
      .type = ZDWM_COMMAND_CONFIGURE_WINDOW,
      .as.configure = {
        .window = interaction->window,
        .changed_fields = ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y,
        .x = rect.x + (end.x - start.x),
        .y = rect.y + (end.y - start.y),
      },
    };
    command_buffer_push(out, &configure);

    interaction->last_change_time = now;
  } break;
  case ZDWM_WINDOW_INTERACTION_RESIZE: {
    auto time = interaction->last_change_time;
    auto now = time_monotonic_ms();
    if (now - time < 1000 / ctx->fps) break;

    auto window = state_window_get(ctx->state, window_id);
    if (!window_can_resize(window)) break;

    auto rect = interaction->origin_rect;
    auto start = interaction->start_coordinate;
    auto end = data->root;

    auto min_size = window->min_size;
    auto max_size = window->max_size;

    auto width = rect.width + (end.x - start.x);
    auto height = rect.height + (end.y - start.y);
    width = MAX(width, min_size.width);
    height = MAX(height, min_size.height);
    width = MIN(width, max_size.width);
    height = MIN(height, max_size.height);

    if (!window_need_resize(window, width, height)) break;

    command_t configure_cmd = {
      .type = ZDWM_COMMAND_CONFIGURE_WINDOW,
      .as.configure = {
        .window = interaction->window,
        .changed_fields =
          ZDWM_CONFIGURE_FIELD_WIDTH | ZDWM_CONFIGURE_FIELD_HEIGHT,
        .width = width,
        .height = height,
      }
    };
    command_buffer_push(out, &configure_cmd);
    interaction->last_change_time = now;
  } break;
  }
}

static void
route_pointer_enter(state_t *state, window_id_t window, command_buffer_t *out) {
  add_focus_window_command(out, window);
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
  if (e->override_redirect) return;
  if (state_window_get(state, e->window)) return;

  window_layer_type_t layer_type = window_classify_layer(&e->props);
  if (e->transient_for != ZDWM_WINDOW_ID_INVALID) {
    const window_t *window = state_window_get(state, e->transient_for);
    if (window) layer_type = MAX(window->layer, layer_type);
  }

  /* clang-format off */
  command_t manage_window_cmd = {
    .type = ZDWM_COMMAND_MANAGE_WINDOW,
    .as.manage_window = {
      .workspace = derive_window_workspace(state),
      .info = {
        .id            = e->window,
        .transient_for = e->transient_for,
        .frame_rect    = e->rect,

        .title         = e->metadata.title,
        .app_id        = e->metadata.app_id,
        .role          = e->metadata.role,
        .class_name    = e->metadata.class_name,
        .instance_name = e->metadata.instance_name,

        .layer_type   = layer_type,
        .fullscreen   = e->fullscreen,
        .maximized    = e->maximized,
        .minimized    = e->minimized,
        .urgent       = e->urgent,
        .skip_taskbar = e->skip_taskbar,
        .min_size     = e->min_size,
        .max_size     = e->max_size,
      },
    },
  };
  /* clang-format on */

  rule_action_t action = {.workspace = ZDWM_WORKSPACE_ID_INVALID};
  bool have_rule_match = rules_resolve(rules, &e->metadata, &action);
  if (have_rule_match) {
    manage_window_command_t *data = &manage_window_cmd.as.manage_window;
    if (!workspace_id_invalid(action.workspace)) {
      data->workspace = action.workspace;
    }
    if (action.floating) data->floating = true;
    if (action.fullscreen) data->info.fullscreen = true;
    if (action.maximize) data->info.maximized = true;
  }

  command_buffer_push(out, &manage_window_cmd);
  if (!have_rule_match || !action.switch_to_workspace) return;

  workspace_id_t workspace_id = manage_window_cmd.as.manage_window.workspace;
  const workspace_t *workspace = state_workspace_get(state, workspace_id);
  if (!workspace) return;

  add_switch_workspace_command(out, workspace_id);
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
      .type = ZDWM_COMMAND_WITHDRAW_WINDOW,
      .as.withdraw.window = e->window,
    };
    command_buffer_push(out, &withdrawn_window_cmd);
    break;
  case ZDWM_WINDOW_REMOVE_DESTROY:
    command_t unmanage_window_cmd = {
      .type = ZDWM_COMMAND_UNMANAGE_WINDOW,
      .as.unmanage.window = e->window,
    };
    command_buffer_push(out, &unmanage_window_cmd);
    break;
  }
}

static void route_window_metadata_changed(
  const policy_context_t *ctx,
  const window_metadata_change_event_t *e
) {
  auto state = ctx->state;

  auto window = state_window_get(state, e->window);
  if (!window) return;

  auto window_id = e->window;
  auto metadata = &e->metadata;

  if (e->changed_fields & ZDWM_WINDOW_METADATA_CHANGE_TITLE) {
    state_window_set_title(state, window_id, metadata->title);
  }
  if (e->changed_fields & ZDWM_WINDOW_METADATA_CHANGE_APP_ID) {
    state_window_set_app_id(state, window_id, metadata->app_id);
  }
  if (e->changed_fields & ZDWM_WINDOW_METADATA_CHANGE_ROLE) {
    state_window_set_role(state, window_id, metadata->role);
  }
  if (e->changed_fields & ZDWM_WINDOW_METADATA_CHANGE_CLASS) {
    state_window_set_class(state, window_id, metadata->class_name);
  }
  if (e->changed_fields & ZDWM_WINDOW_METADATA_CHANGE_INSTANCE) {
    state_window_set_instance(state, window_id, metadata->instance_name);
  }
  listeners_notify_window_updated(ctx->listeners, state, window_id);
}

static void route_window_hints_changed(
  state_t *state,
  const hints_data_t *data,
  command_buffer_t *out
) {
  auto window = (window_t *)state_window_get(state, data->window);
  if (!window) return;

  command_t change_hints_cmd = {
    .type = ZDWM_COMMAND_CHANGE_HINTS,
    .as.hints = *data,
  };
  command_buffer_push(out, &change_hints_cmd);
}

static void route_window_activate_request(
  state_t *state,
  const window_activate_request_event_t *e,
  command_buffer_t *out
) {
  auto window = (window_t *)state_window_get(state, e->window);
  if (!window) return;

  switch (e->source) {
  case ZDWM_WINDOW_ACTIVATION_SOURCE_LEGACY:
    break;
  case ZDWM_WINDOW_ACTIVATION_SOURCE_APPLICATION:
    window->urgent = true;
    break;
  case ZDWM_WINDOW_ACTIVATION_SOURCE_PAGER:
    add_switch_workspace_command(out, window->workspace_id);
    add_focus_window_command(out, window->id);
    break;
  }
}

static void route_window_state_request(
  state_t *state,
  const window_state_request_event_t *e,
  command_buffer_t *out
) {
  auto window = state_window_get(state, e->window);
  if (!window) return;

  command_t change_window_state_cmd = {
    .type = ZDWM_COMMAND_CHANGE_WINDOW_STATE,
    .as.state_change = {
      .type = e->type,
      .window = e->window,
      .action = e->action,
    }
  };
  command_buffer_push(out, &change_window_state_cmd);
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
      .type = ZDWM_COMMAND_CONFIGURE_WINDOW,
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
    .type = ZDWM_COMMAND_CONFIGURE_WINDOW,
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
  case ZDWM_EVENT_NONE:
    break;
  case ZDWM_EVENT_KEY_PRESS:
    route_key_press(ctx, &event->as.key_press, out);
    break;
  case ZDWM_EVENT_POINTER_BUTTON_PRESS:
    route_pointer_press(ctx, &event->as.pointer_button_press, out);
    break;
  case ZDWM_EVENT_POINTER_BUTTON_RELEASE:
    route_pointer_release(ctx, &event->as.pointer_button_release, out);
    break;
  case ZDWM_EVENT_POINTER_MOTION:
    route_pointer_motion(ctx, &event->as.pointer_motion, out);
    break;
  case ZDWM_EVENT_POINTER_ENTER:
    route_pointer_enter(state, event->as.pointer_enter.window, out);
    break;
  case ZDWM_EVENT_WINDOW_MAP_REQUEST:
    route_map_request(state, ctx->rules, &event->as.window_map_request, out);
    break;
  case ZDWM_EVENT_WINDOW_REMOVE:
    route_window_remove(state, &event->as.window_remove, out);
    break;
  case ZDWM_EVENT_WINDOW_METADATA_CHANGED:
    route_window_metadata_changed(ctx, &event->as.window_metadata_change);
    break;
  case ZDWM_EVENT_WINDOW_HINTS_CHANGED:
    route_window_hints_changed(state, &event->as.hints, out);
    break;
  case ZDWM_EVENT_WINDOW_ACTIVATE_REQUEST: {
    auto data = &event->as.window_activate_request;
    route_window_activate_request(state, data, out);
  } break;
  case ZDWM_EVENT_WINDOW_STATE_REQUEST:
    route_window_state_request(state, &event->as.window_state_request, out);
    break;
  case ZDWM_EVENT_CONFIGURE_REQUEST: {
    auto data = &event->as.configure_request;
    route_configure_request(state, data, ctx->layouts, out);
  } break;
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
  auto state = ctx->state;
  auto border = ctx->border;

  auto workspace = state_workspace_get(state, workspace_id);
  if (!workspace) return;

  state_workspace_set_focused_window(state, workspace_id, window_id);
  plan_push_focus_effect(plan, window_id);

  if (state_window_get(state, window_id)) {
    auto color = &border->focused_color;
    plan_push_change_border_color_effect(plan, window_id, color);
  }
  for (size_t i = 0; i < state_window_count(state); ++i) {
    auto window = state_window_at(state, i);
    if (window->workspace_id != workspace_id) continue;
    if (workspace->focused_window_id == window->id) continue;
    auto color = &border->normal_color;
    plan_push_change_border_color_effect(plan, window->id, color);
  }
}

static void push_window_list_effect(const state_t *state, plan_t *plan) {
  window_list_t list = {0};
  for (size_t i = 0; i < state_window_count(state); ++i) {
    auto window = state_window_at(state, i);
    window_list_push(&list, window->id);
  }
  effect_t effect = {
    .type = ZDWM_EFFECT_CHANGE_WINDOW_LIST,
    .as.change_window_list = {
      .windows = list.windows,
      .count = list.count,
    },
  };
  plan_push_effect(plan, &effect);
}

static void plan_push_grab_button_effect(plan_t *plan, window_id_t window) {
  /* TODO: 后续会改成配置 */
  static const grab_button_t buttons[] = {
    [0] = {.modifiers = ZDWM_MOD_4, .button = ZDWM_BUTTON_LEFT},
    [1] = {.modifiers = ZDWM_MOD_4, .button = ZDWM_BUTTON_RIGHT},
  };

  constexpr const size_t count = countof(buttons);
  auto btns = p_new(grab_button_t, count);
  memcpy(btns, buttons, sizeof(buttons));

  effect_t grab_button_effect = {
    .type = ZDWM_EFFECT_GRAB_BUTTON,
    .as.grab_button = {.window = window, .buttons = btns, .count = count},
  };
  plan_push_effect(plan, &grab_button_effect);
}

static void manage_window(
  const policy_context_t *ctx,
  const manage_window_command_t *command,
  plan_t *plan
) {
  auto state = ctx->state;
  auto window = state_window_add(state, &command->info);
  auto window_id = window->id;
  auto workspace_id = command->workspace;
  state_window_set_workspace(state, window_id, workspace_id);
  set_foucs_window(ctx, workspace_id, window_id, plan);
  plan_push_grab_button_effect(plan, window_id);
  push_window_list_effect(state, plan);
  listeners_notify_window_added(ctx->listeners, state, window_id);

  if (window_should_fix_size(window)) {
    state_window_set_floating(state, window_id, true);
  } else {
    state_window_set_floating(state, window_id, command->floating);
  }

  auto workspace = state_workspace_get(state, workspace_id);
  auto need_layout = window_need_layout(window);
  auto layout_func = layout_get(ctx->layouts, workspace->layout_id);

  if (need_layout && layout_func) {
    adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
  } else {
    state_window_set_border_width(state, window_id, ctx->border->width);
  }

  if (!state_workspace_show(state, command->workspace)) return;

  plan_push_map_effect(plan, window_id);
  plan_push_focus_effect(plan, window_id);

  plan->need_relayout = true;

  effect_t configure_effect = {
    .type = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window = window_id,
      .x = window->frame_rect.x,
      .y = window->frame_rect.y,
      .width = window->frame_rect.width - 2 * (int32_t)window->border_width,
      .height = window->frame_rect.height - 2 * (int32_t)window->border_width,
      .border_width = window->border_width,
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
  auto state = ctx->state;
  const window_t *win = state_window_get(state, window);
  if (!win) return;

  auto workspace_id = win->workspace_id;
  auto workspace = state_workspace_get(state, workspace_id);
  auto output = state_output_get(state, workspace->output_id);
  auto need_layout = window_need_layout(win);

  auto old_focused_window = workspace->focused_window_id;
  state_window_remove(state, window);

  push_window_list_effect(state, plan);
  listeners_notify_window_removed(ctx->listeners, window);

  if (need_layout) {
    adjust_layout_windows_border_width(state, ctx->border->width, workspace_id);
    plan->need_relayout = true;
  }

  if (output->current_workspace_id != workspace_id) return;

  auto focused_window_id = workspace->focused_window_id;
  if (old_focused_window != focused_window_id) {
    plan_push_focus_effect(plan, focused_window_id);
    listeners_notify_window_updated(ctx->listeners, state, focused_window_id);
  }
}

static void
focus_window(const policy_context_t *ctx, window_id_t window, plan_t *plan) {
  auto state = ctx->state;
  auto win = state_window_get(state, window);
  if (!win) return;

  auto workspace_id = win->workspace_id;
  auto workspace = state_workspace_get(state, workspace_id);
  auto old_focused_window = workspace->focused_window_id;

  set_foucs_window(ctx, workspace_id, window, plan);
  if (old_focused_window == workspace->focused_window_id) return;

  plan_push_focus_effect(plan, workspace->focused_window_id);
  listeners_notify_window_updated(ctx->listeners, state, window);
  listeners_notify_window_updated(ctx->listeners, state, old_focused_window);
}

static void kill_window(state_t *state, window_id_t window, plan_t *plan) {
  auto win = state_window_get(state, window);
  if (!win) return;

  plan_push_kill_effect(plan, window);
}

static void raise_window(state_t *state, window_id_t window, plan_t *plan) {
  if (!state_stack_raise(state, window)) return;

  window_list_t list = {0};
  for (size_t i = 0; i < countof(state->stacks); ++i) {
    for (size_t j = 0; j < state->stacks[i].count; ++j) {
      auto window_id = state->stacks[i].order[j];
      window_list_push(&list, window_id);
    }
  }
  effect_t restack_windows_effect = {
    .type = ZDWM_EFFECT_RESTACK_WINDOWS,
    .as.restack_windows = {
      .windows = list.windows,
      .count = list.count,
    },
  };
  plan_push_effect(plan, &restack_windows_effect);
}

static void
withdraw_window(const policy_context_t *ctx, window_id_t window, plan_t *plan) {
  auto state = ctx->state;

  auto win = state_window_get(state, window);
  if (!win) return;

  plan_push_withdraw_effect(plan, window);
  listeners_notify_window_removed(ctx->listeners, window);
}

static void
configure_window(state_t *state, const configure_data_t *data, plan_t *plan) {
  auto window = state_window_get(state, data->window);
  if (!window) {
    effect_t configure_effect = {
      .type = ZDWM_EFFECT_CONFIGURE_WINDOW,
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

  auto need_move = window_need_move(window, rect.x, rect.y);
  auto need_resize = window_need_resize(window, rect.width, rect.height);
  if (!need_move && !need_resize) return;

  state_window_set_frame_rect(state, window->id, rect);

  effect_t configure_effect = {
    .type = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window = window->id,
      .x = rect.x,
      .y = rect.y,
      .width = rect.width,
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
);
static void maximize_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
);

static void add_window_floating_effect(
  const policy_context_t *ctx,
  const window_t *window,
  plan_t *plan
) {
  effect_t configure_effect = {
    .type = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window = window->id,
      .border_width = ctx->border->width,
      .x = window->frame_rect.x,
      .y = window->frame_rect.y,
      .width = window->frame_rect.width - 2 * (int32_t)ctx->border->width,
      .height = window->frame_rect.height - 2 * (int32_t)ctx->border->width,
      .changed_fields = ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y |
                        ZDWM_CONFIGURE_FIELD_WIDTH |
                        ZDWM_CONFIGURE_FIELD_HEIGHT |
                        ZDWM_CONFIGURE_FIELD_BORDER_WIDTH
    }
  };
  plan_push_effect(plan, &configure_effect);
}

static void floating_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto window = state_window_get(ctx->state, window_id);
  if (!window) return;

  bool old_floating = window->floating;
  state_window_set_floating(ctx->state, window_id, value);
  if (old_floating == window->floating) return;

  if (window->floating) add_window_floating_effect(ctx, window, plan);

  plan->need_relayout = true;
}

static void fullscreen_window(
  const policy_context_t *ctx,
  window_id_t window_id,
  bool value,
  plan_t *plan
) {
  auto state = ctx->state;
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if (window->fullscreen == value) return;

  plan_push_fullscreen_effect(plan, window_id, value);

  state_window_set_fullscreen(state, window_id, value);
  if (value) {
    state_window_set_border_width(state, window_id, 0);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
      add_window_floating_effect(ctx, window, plan);
    }
  } else {
    state_window_set_border_width(state, window_id, ctx->border->width);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
      add_window_floating_effect(ctx, window, plan);
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

  if (window->maximized == value) return;

  plan_push_maximize_effect(plan, window_id, value);

  state_window_set_maximized(state, window_id, value);
  if (value) {
    state_window_set_border_width(state, window_id, 0);
    if (window->floating) {
      state_window_set_float_rect(state, window_id, window->frame_rect);
      add_window_floating_effect(ctx, window, plan);
    }
  } else {
    state_window_set_border_width(state, window_id, ctx->border->width);
    if (window->floating) {
      state_window_set_frame_rect(state, window_id, window->float_rect);
      add_window_floating_effect(ctx, window, plan);
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
  auto state = ctx->state;
  auto window = state_window_get(state, window_id);
  if (!window) return;

  if (window->minimized == value) return;

  plan_push_minimize_effect(plan, window_id, value);

  state_window_set_minimized(state, window_id, value);
  if (value) {
    plan_push_unmap_effect(plan, window_id);

    auto workspace = state_workspace_get(state, window->workspace_id);
    auto old_focused_window_id = workspace->focused_window_id;
    auto new_focused_window_id = workspace->focused_window_id;
    if (old_focused_window_id != new_focused_window_id) {
      auto color = &ctx->border->normal_color;
      plan_push_change_border_color_effect(plan, old_focused_window_id, color);

      color = &ctx->border->focused_color;
      plan_push_change_border_color_effect(plan, new_focused_window_id, color);
    }
  } else {
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
      value = !window->fullscreen;
      break;
    case ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED:
      value = !window->maximized;
      break;
    case ZDWM_WINDOW_STATE_REQUEST_MINIMIZED:
      value = !window->minimized;
      break;
    case ZDWM_WINDOW_STATE_REQUEST_SKIP_TASKBAR:
      value = !window->skip_taskbar;
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
  /* TODO: 下面三个可能需要添加处理，尤其是 fixed_size 和 floating 相关 */
  case ZDWM_WINDOW_STATE_REQUEST_SKIP_TASKBAR:
    state_window_set_skip_taskbar(state, window->id, value);
    break;
  }

  listeners_notify_window_updated(ctx->listeners, state, command->window);
}

static void start_window_move(
  const policy_context_t *ctx,
  const start_interaction_command_t *command,
  plan_t *plan
) {
  auto window_id = command->window;
  auto window = state_window_get(ctx->state, window_id);
  if (!window) return;

  plan_push_move_effect(plan, window_id);

  *ctx->interaction = (window_interaction_state_t){
    .mode = ZDWM_WINDOW_INTERACTION_MOVE,
    .window = window_id,
    .start_coordinate = command->pointer_coordinate,
    .origin_rect = window->frame_rect,
  };
}

static void start_window_resize(
  const policy_context_t *ctx,
  const start_interaction_command_t *command,
  plan_t *plan
) {
  auto window_id = command->window;
  auto window = state_window_get(ctx->state, window_id);
  if (!window) return;

  auto frame_rect = window->frame_rect;

  point_t pointer_position = {
    .x = frame_rect.x + frame_rect.width,
    .y = frame_rect.y + frame_rect.height,
  };

  plan_push_resize_effect(plan, window_id);

  *ctx->interaction = (window_interaction_state_t){
    .mode = ZDWM_WINDOW_INTERACTION_RESIZE,
    .window = window_id,
    .start_coordinate = pointer_position,
    .origin_rect = frame_rect,
  };
}

static void send_window_to_workspace(
  const policy_context_t *ctx,
  const window_send_to_workspace_command_t *command,
  plan_t *plan
) {
  auto state = ctx->state;
  auto window = state_window_get(state, command->window);
  if (!window) return;
  if (!state_workspace_valid(state, command->workspace)) return;
  if (window->workspace_id == command->workspace) return;

  workspace_id_t src_workspace_id = window->workspace_id;
  bool src_visible = state_workspace_show(state, src_workspace_id);
  bool dst_visible = state_workspace_show(state, command->workspace);

  const workspace_t *src_ws = state_workspace_get(state, src_workspace_id);
  window_id_t src_old_focused =
    src_ws ? src_ws->focused_window_id : ZDWM_WINDOW_ID_INVALID;

  state_window_set_workspace(state, command->window, command->workspace);

  if (!window->sticky) {
    if (src_visible) plan_push_unmap_effect(plan, command->window);
    if (dst_visible) plan_push_map_effect(plan, command->window);
  }

  if (src_visible) {
    if (src_ws && src_ws->focused_window_id != src_old_focused) {
      if (
        !window_id_invalid(src_old_focused) &&
        state_window_get(state, src_old_focused)
      ) {
        plan_push_change_border_color_effect(
          plan,
          src_old_focused,
          &ctx->border->normal_color
        );
      }
      if (
        !window_id_invalid(src_ws->focused_window_id) &&
        state_window_get(state, src_ws->focused_window_id)
      ) {
        plan_push_focus_effect(plan, src_ws->focused_window_id);
        plan_push_change_border_color_effect(
          plan,
          src_ws->focused_window_id,
          &ctx->border->focused_color
        );
      }
    }
    adjust_layout_windows_border_width(
      state,
      ctx->border->width,
      src_workspace_id
    );
  }

  if (dst_visible) {
    set_foucs_window(ctx, command->workspace, command->window, plan);
    adjust_layout_windows_border_width(
      state,
      ctx->border->width,
      command->workspace
    );
  }

  plan->need_relayout = true;

  auto listeners = ctx->listeners;
  listeners_notify_window_updated(listeners, state, command->window);
  if (
    src_ws && src_ws->focused_window_id != src_old_focused &&
    !window_id_invalid(src_ws->focused_window_id)
  ) {
    listeners_notify_window_updated(
      listeners,
      state,
      src_ws->focused_window_id
    );
  }
}

static void command_window_set_floating(
  const policy_context_t *ctx,
  const window_bool_state_t *data,
  plan_t *plan
) {
  floating_window(ctx, data->window, data->state, plan);
}

static void command_window_set_sticky(
  const policy_context_t *ctx,
  const window_bool_state_t *data,
  plan_t *plan
) {
  auto window = state_window_get(ctx->state, data->window);
  if (!window || window->sticky == data->state) return;

  if (!window->floating) {
  }
  state_window_set_sticky(ctx->state, data->window, data->state);
  if (data->state) return;
}

static void command_window_set_minimized(
  const policy_context_t *ctx,
  const window_bool_state_t *data,
  plan_t *plan
) {
  minimize_window(ctx, data->window, data->state, plan);
}

static void command_window_set_maximized(
  const policy_context_t *ctx,
  const window_bool_state_t *data,
  plan_t *plan
) {
  maximize_window(ctx, data->window, data->state, plan);
}

static void
command_change_hints(state_t *state, hints_data_t *data, plan_t *plan) {
  auto window = (window_t *)state_window_get(state, data->window);
  if (data->changed_fields & ZDWM_HINT_FIELD_URGENT) {
    window->urgent = data->urgent;
  }
  if (data->changed_fields & ZDWM_HINT_FIELD_SIZE) {
    window->max_size = data->max_size;
    window->min_size = data->min_size;

    auto floating = window->floating;
    if (window_should_fix_size(window)) window_set_floating(window, true);
    if (floating != window->floating) plan->need_relayout = true;
  }
}

static inline void change_current_output(
  state_t *state,
  output_id_t output_id,
  const listeners_t *listeners
) {
  if (!state_set_current_output(state, output_id)) return;
  listeners_notify_current_output(listeners, output_id);
}

static void switch_workspace(
  const policy_context_t *ctx,
  workspace_id_t workspace_id,
  plan_t *plan
) {
  auto state = ctx->state;
  auto workspace = state_workspace_get(state, workspace_id);
  if (!workspace) return;

  auto listeners = ctx->listeners;
  auto output_id = workspace->output_id;
  workspace_id_t old_workspace = ZDWM_WORKSPACE_ID_INVALID;

  if (
    state_output_set_current_workspace(
      state,
      output_id,
      workspace_id,
      &old_workspace
    )
  ) {
    plan->need_relayout = true;
    add_switch_workspace_effects(state, old_workspace, workspace_id, plan);
    listeners_notify_workspace_active(listeners, output_id, workspace_id);
  }

  change_current_output(state, output_id, listeners);
}

static void set_current_output(
  const policy_context_t *ctx,
  const set_current_output_command_t *command
) {
  change_current_output(ctx->state, command->output, ctx->listeners);
}

static void set_layout(
  const policy_context_t *ctx,
  const set_layout_command_t *command,
  plan_t *plan
) {
  auto state = ctx->state;
  auto workspace_id = command->workspace;
  auto layout_id = command->layout;

  if (!state_workspace_set_layout_by_id(state, workspace_id, layout_id)) return;

  plan->need_relayout = true;
  listeners_notify_layout(
    ctx->listeners,
    ctx->layouts,
    workspace_id,
    layout_id
  );
}

static void set_binding_mode(
  const policy_context_t *ctx,
  zdwm_binding_mode_id_t binding_mode,
  plan_t *plan
) {
  if (!binding_table_set_current_mode(ctx->bind_table, binding_mode)) return;

  size_t count = 0;
  auto bindings = binding_table_get_current_bindings(ctx->bind_table, &count);
  if (bindings && count > 0) {
    auto keys = p_new(key_bind_t, count);
    for (size_t i = 0; i < count; ++i) {
      keys[i].keysym = bindings[i].keysym;
      keys[i].modifiers = bindings[i].modifiers;
    }

    effect_t effect = {
      .type = ZDWM_EFFECT_BIND_KEY,
      .as.bind_key = {.count = count, .keys = keys},
    };
    plan_push_effect(plan, &effect);
  }

  listeners_notify_binding_mode(ctx->listeners, ctx->bind_table);
}

static void
set_bar_visibility(const policy_context_t *ctx, bool visible, plan_t *plan) {
  auto bar = &ctx->bar;
  if (*bar->visible == visible) return;

  *bar->visible = visible;

  inset_t inset = {0};
  if (visible) {
    if (bar->show_top) inset.top = bar->height;
    else inset.bottom = bar->height;
  }

  auto state = ctx->state;
  for (size_t i = 0; i < state->output_count; ++i) {
    auto output = state_output_at(state, i);
    state_output_inset_workarea(state, output->id, inset);
  }

  auto push_effect = visible ? plan_push_map_effect : plan_push_unmap_effect;
  for (size_t i = 0; i < bar->windows->count; ++i) {
    auto window_id = bar->windows->windows[i];
    push_effect(plan, window_id);
  }

  plan->need_relayout = true;
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
    case ZDWM_COMMAND_RAISE_WINDOW:
      raise_window(state, cmd->as.raise.window, plan);
      break;
    case ZDWM_COMMAND_WITHDRAW_WINDOW:
      withdraw_window(ctx, cmd->as.withdraw.window, plan);
      break;
    case ZDWM_COMMAND_CONFIGURE_WINDOW:
      configure_window(state, &cmd->as.configure, plan);
      break;
    case ZDWM_COMMAND_CHANGE_WINDOW_STATE:
      change_window_state(ctx, &cmd->as.state_change, plan);
      break;
    case ZDWM_COMMAND_START_MOVE_WINDOW:
      start_window_move(ctx, &cmd->as.move, plan);
      break;
    case ZDWM_COMMAND_START_RESIZE_WINDOW:
      start_window_resize(ctx, &cmd->as.resize, plan);
      break;
    case ZDWM_COMMAND_STOP_MOVE_WINDOW:
    case ZDWM_COMMAND_STOP_RESIZE_WINDOW:
      plan_push_ungrab_pointer(plan);
      p_clear(ctx->interaction, 1);
      break;
    case ZDWM_COMMAND_WINDOW_SEND_TO_WORKSPACE:
      send_window_to_workspace(ctx, &cmd->as.send_to_workspace, plan);
      break;
    case ZDWM_COMMAND_WINDOW_SET_FLOATING:
      command_window_set_floating(ctx, &cmd->as.floating, plan);
      break;
    case ZDWM_COMMAND_WINDOW_SET_STICKY:
      command_window_set_sticky(ctx, &cmd->as.sticky, plan);
      break;
    case ZDWM_COMMAND_WINDOW_SET_MINIMIZED:
      command_window_set_minimized(ctx, &cmd->as.minimized, plan);
      break;
    case ZDWM_COMMAND_WINDOW_SET_MAXIMIZED:
      command_window_set_maximized(ctx, &cmd->as.maximized, plan);
      break;
    case ZDWM_COMMAND_WINDOW_SET_FULLSCREEN: {
      auto window = cmd->as.fullscreen.window;
      auto state = cmd->as.fullscreen.state;
      fullscreen_window(ctx, window, state, plan);
    } break;
    case ZDWM_COMMAND_CHANGE_HINTS:
      command_change_hints(state, &cmd->as.hints, plan);
      break;
    case ZDWM_COMMAND_SWITCH_WORKSPACE:
      switch_workspace(ctx, cmd->as.switch_workspace.workspace, plan);
      break;
    case ZDWM_COMMAND_SET_CURRENT_OUTPUT:
      set_current_output(ctx, &cmd->as.current_output);
      break;
    case ZDWM_COMMAND_SET_LAYOUT:
      set_layout(ctx, &cmd->as.layout, plan);
      break;
    case ZDWM_COMMAND_SET_BINDING_MODE:
      set_binding_mode(ctx, cmd->as.binding_mode.mode, plan);
      break;
    case ZDWM_COMMAND_SET_BAR_VISIBILITY:
      set_bar_visibility(ctx, cmd->as.bar_visibility.visible, plan);
      break;
    case ZDWM_COMMAND_QUIT:
      plan->quit = true;
      plan->will_restart = cmd->as.quit.will_restart;
      break;
    }
  }
}
