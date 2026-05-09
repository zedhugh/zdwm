#include "core/action.h"

#include <paths.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/log.h"
#include "base/macros.h"
#include "core/command.h"
#include "core/command_buffer.h"
#include "core/runtime.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"

void spawn(const char *command) {
  if (fork() == 0) {
    setsid();

    if (fork() == 0) {
      execl(_PATH_BSHELL, _PATH_BSHELL, "-c", command, nullptr);
      fatal("execl fail: %s", command);
    }

    exit(0);
  }

  wait(0);
}

void quit(runtime_t *runtime, bool restart) {
  runtime->will_restart = restart;
  runtime->running      = false;
}

static const window_t *
get_next_window_by_class(state_t *state, const char *class_name) {
  auto output    = state_output_at(state, state->current_output_index);
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

void raise_or_run(
  runtime_t *runtime,
  const char *class_name,
  const char *command
) {
  auto window = get_next_window_by_class(&runtime->state, class_name);
  if (!window) {
    spawn(command);
    return;
  }

  command_t switch_workspace_cmd = {
    .type                          = ZDWM_COMMAND_SWITCH_WORKSPACE,
    .as.switch_workspace.workspace = window->workspace_id,
  };
  command_buffer_push(&runtime->command_buffer, &switch_workspace_cmd);

  command_t focus_cmd = {
    .type            = ZDWM_COMMAND_FOCUS_WINDOW,
    .as.focus.window = window->id,
  };
  command_buffer_push(&runtime->command_buffer, &focus_cmd);
}

static const window_t *get_current_focused_window(const state_t *state) {
  auto output    = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);
  return state_window_get(state, workspace->focused_window_id);
}

void toggle_fullscreen(runtime_t *runtime) {
  auto window = get_current_focused_window(&runtime->state);
  if (!window) return;

  command_t window_set_fullscreen_cmd = {
    .type          = ZDWM_COMMAND_WINDOW_SET_FULLSCREEN,
    .as.fullscreen = {
      .window = window->id,
      .state  = !window->fullscreen,
    }
  };
  command_buffer_push(&runtime->command_buffer, &window_set_fullscreen_cmd);
}

void toggle_maximize(runtime_t *runtime) {
  auto window = get_current_focused_window(&runtime->state);
  if (!window) return;

  command_t window_set_maximized_cmd = {
    .type         = ZDWM_COMMAND_WINDOW_SET_MAXIMIZED,
    .as.maximized = {
      .window = window->id,
      .state  = !window->maximized,
    }
  };
  command_buffer_push(&runtime->command_buffer, &window_set_maximized_cmd);
}

void toggle_floating(runtime_t *runtime) {
  auto window = get_current_focused_window(&runtime->state);
  if (!window) return;

  command_t window_set_floating_cmd = {
    .type        = ZDWM_COMMAND_WINDOW_SET_FLOATING,
    .as.floating = {
      .window = window->id,
      .state  = !window->floating,
    }
  };
  command_buffer_push(&runtime->command_buffer, &window_set_floating_cmd);
}

void toggle_sticky(runtime_t *runtime) {
  auto window = get_current_focused_window(&runtime->state);
  if (!window) return;

  command_t window_set_sticky_cmd = {
    .type      = ZDWM_COMMAND_WINDOW_SET_STICKY,
    .as.sticky = {
      .window = window->id,
      .state  = !window->sticky,
    }
  };
  command_buffer_push(&runtime->command_buffer, &window_set_sticky_cmd);
}

void switch_workspace(runtime_t *runtime, workspace_id_t workspace_id) {
  auto workspace = state_workspace_get(&runtime->state, workspace_id);
  if (!workspace) return;

  command_t switch_workspace_cmd = {
    .type                          = ZDWM_COMMAND_SWITCH_WORKSPACE,
    .as.switch_workspace.workspace = workspace_id
  };
  command_buffer_push(&runtime->command_buffer, &switch_workspace_cmd);
}

void switch_workspace_in_current_output(
  runtime_t *runtime,
  workspace_id_t workspace_id
) {
  auto state     = &runtime->state;
  auto output    = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, workspace_id);
  if (workspace->output_id != output->id) return;

  command_t switch_workspace_cmd = {
    .type                          = ZDWM_COMMAND_SWITCH_WORKSPACE,
    .as.switch_workspace.workspace = workspace_id
  };
  command_buffer_push(&runtime->command_buffer, &switch_workspace_cmd);
}

void switch_workspace_by_index_in_current_output(
  runtime_t *runtime,
  uint32_t index
) {
  auto state     = &runtime->state;
  auto output    = state_output_at(state, state->current_output_index);
  uint32_t count = 0;
  for (size_t i = 0; i < state_window_count(state); ++i) {
    auto workspace = state_workspace_at(state, i);
    if (workspace->output_id != output->id) continue;

    if (count == index) {
      command_t switch_workspace_cmd = {
        .type                          = ZDWM_COMMAND_SWITCH_WORKSPACE,
        .as.switch_workspace.workspace = workspace->id,
      };
      command_buffer_push(&runtime->command_buffer, &switch_workspace_cmd);
      return;
    }
    count++;
  }
}

void cycle_layout(runtime_t *runtime, int32_t delta) {
  auto state     = &runtime->state;
  auto output    = state_output_at(state, state->current_output_index);
  auto workspace = state_workspace_get(state, output->current_workspace_id);
  if (!state_workspace_cycle_layout(state, workspace->id, delta)) return;

  runtime->plan.need_relayout = true;
}

void cycle_current_output(runtime_t *runtime, int32_t delta) {
  auto state = &runtime->state;
  state_cycle_current_output(state, delta);
}

void focus_window(runtime_t *runtime, int32_t delta) {
  auto state     = &runtime->state;
  auto output    = state_output_at(state, state->current_output_index);
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

  command_t focus_cmd = {
    .type            = ZDWM_COMMAND_FOCUS_WINDOW,
    .as.focus.window = target->id,
  };
  command_buffer_push(&runtime->command_buffer, &focus_cmd);

  command_t raise_cmd = {
    .type            = ZDWM_COMMAND_RAISE_WINDOW,
    .as.raise.window = target->id,
  };
  command_buffer_push(&runtime->command_buffer, &raise_cmd);
}
