#include "core/action.h"

#include <paths.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/log.h"
#include "base/macros.h"
#include "core/command.h"
#include "core/command_buffer.h"
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
  zdwm_runtime_t *runtime,
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
