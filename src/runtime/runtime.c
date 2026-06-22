#include "runtime/runtime.h"

#include <bits/types/sigset_t.h>
#include <dlfcn.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <zdwm/layout.h>

#include "bar/bar.h"
#include "base/array.h"
#include "base/color.h"
#include "base/log.h"
#include "base/macros.h"
#include "base/memory.h"
#include "base/window_list.h"
#include "config/runtime_config.h"
#include "core/backend.h"
#include "core/binding.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/layout.h"
#include "core/listeners.h"
#include "core/plan.h"
#include "core/policy.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

typedef struct runtime_t {
  bool running;
  bool will_restart;
  int signal_fd;

  plan_t plan;
  command_buffer_t command_buffer;
  state_t state;
  layout_result_t layout_result;
  layout_registry_t layouts;
  rules_t rules;
  border_config_t border;
  backend_t *backend;
  void *config_module_handle;
  binding_table_t *binding_table;
  listeners_t listeners;
  window_interaction_state_t interaction;

  bar_t bar;
  window_list_t bar_windows;
} runtime_t;

static bool runtime_workspace_desc_has_valid_layouts(
  const layout_registry_t *layouts,
  const workspace_desc_t *workspace
) {
  if (!layouts || !workspace) return false;

  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (!layout_slot_get(layouts, workspace->layout_ids[i])) return false;
  }

  return true;
}

static bool runtime_init_desc_valid(const runtime_init_desc_t *desc) {
  if (!desc || !desc->backend || !desc->outputs || desc->output_count == 0) {
    return false;
  }
  if (!desc->layouts.slots || desc->layouts.slot_count == 0) return false;
  if (!desc->workspaces || desc->workspace_count == 0) return false;

  for (size_t i = 0; i < desc->workspace_count; i++) {
    const workspace_desc_t *workspace = &desc->workspaces[i];
    if (!workspace_desc_valid(workspace, desc->output_count)) return false;
    if (!runtime_workspace_desc_has_valid_layouts(&desc->layouts, workspace)) {
      return false;
    }
  }

  return true;
}

static void runtime_init_bar(runtime_t *runtime) {
  auto bar_height = runtime->bar.config.height;
  if (bar_height <= 0) return;

  auto show_top = runtime->bar.config.show_top;
  inset_t inset = {
    .top    = show_top ? bar_height : 0,
    .bottom = show_top ? 0 : bar_height,
  };

  auto state = &runtime->state;
  auto count = state_output_count(state);

  auto bar     = &runtime->bar;
  bar->bars    = p_new(bar_output_t, count);
  bar->count   = count;
  bar->visible = true;

  color_parse(bar->config.bg, &bar->palette.bg);

  auto backend = runtime->backend;
  for (size_t i = 0; i < count; ++i) {
    auto output = state_output_at(state, i);
    state_output_inset_workarea(state, output->id, inset);
    auto geometry   = output->geometry;
    rect_t bar_rect = {
      .x      = geometry.x,
      .y      = show_top ? 0 : geometry.y + geometry.height - bar_height,
      .width  = geometry.width,
      .height = bar_height,
    };
    auto color            = bar->palette.bg.argb;
    auto bar_window       = backend_create_bar_window(backend, bar_rect, color);
    auto bar_output       = &bar->bars[i];
    bar_output->cr        = bar_window.cr;
    bar_output->window_id = bar_window.window_id;
    bar_output->output_id = output->id;
    bar_output->width     = bar_rect.width;
    bar_output->height    = bar_rect.height;
  }

  bar_init(&runtime->bar, &runtime->listeners);
}

static bool runtime_init(runtime_t *runtime, runtime_init_desc_t *desc) {
  if (!runtime || !runtime_init_desc_valid(desc)) return false;

  p_clear(runtime, 1);
  runtime->backend = desc->backend;
  layout_registry_move(&desc->layouts, &runtime->layouts);
  rules_move(&desc->rules, &runtime->rules);
  runtime->signal_fd            = -1;
  runtime->border               = desc->border;
  runtime->config_module_handle = desc->config_module_handle;
  runtime->binding_table        = desc->binding_table;
  runtime->listeners            = desc->listeners;
  runtime->bar.config           = desc->bar;
  desc->backend                 = nullptr;
  desc->config_module_handle    = nullptr;
  desc->binding_table           = nullptr;
  p_clear(&desc->listeners, 1);
  p_clear(&desc->bar, 1);

  state_init(
    &runtime->state,
    desc->outputs,
    desc->output_count,
    desc->workspaces,
    desc->workspace_count
  );

  workspace_desc_list_cleanup(&desc->workspaces, &desc->workspace_count);
  desc->outputs      = nullptr;
  desc->output_count = 0;

  return true;
}

static void runtime_init_desc_cleanup(runtime_init_desc_t *desc) {
  if (!desc) return;

  binding_table_destroy(desc->binding_table);
  desc->binding_table = nullptr;
  listeners_cleanup(&desc->listeners);

  if (desc->backend) {
    backend_destroy(desc->backend);
    desc->backend = nullptr;
  }

  if (desc->layouts.slots || desc->layouts.slot_count ||
      desc->layouts.slot_capacity) {
    layout_registry_cleanup(&desc->layouts);
  }

  workspace_desc_list_cleanup(&desc->workspaces, &desc->workspace_count);
  desc->outputs      = nullptr;
  desc->output_count = 0;
  if (desc->config_module_handle) dlclose(desc->config_module_handle);
  desc->config_module_handle = nullptr;
}

static void runtime_shutdown(runtime_t *runtime) {
  runtime->running = false;
  plan_cleanup(&runtime->plan);
  command_buffer_cleanup(&runtime->command_buffer);
  layout_registry_cleanup(&runtime->layouts);
  rules_cleanup(&runtime->rules);
  state_cleanup(&runtime->state);
  layout_result_cleanup(&runtime->layout_result);
  binding_table_destroy(runtime->binding_table);
  runtime->binding_table = nullptr;
  listeners_cleanup(&runtime->listeners);
  p_clear(&runtime->interaction, 1);
  window_list_cleanup(&runtime->bar_windows);
  backend_destroy(runtime->backend);
  runtime->backend = nullptr;
  if (runtime->config_module_handle) dlclose(runtime->config_module_handle);
  runtime->config_module_handle = nullptr;

  bar_cleanup(&runtime->bar);

  close(runtime->signal_fd);
  runtime->signal_fd = -1;
}

static void runtime_notify_initial_state(runtime_t *runtime) {
  auto listeners = &runtime->listeners;
  auto state     = &runtime->state;

  auto output = state_output_at(state, state->current_output_index);
  if (output) listeners_notify_current_output(listeners, output->id);

  listeners_notify_initial_workspaces(listeners, state);

  for (size_t i = 0; i < state_output_count(state); ++i) {
    auto item = state_output_at(state, i);
    if (!item) continue;

    listeners_notify_workspace_active(
      listeners,
      item->id,
      item->current_workspace_id
    );
  }

  for (size_t i = 0; i < state_workspace_count(state); ++i) {
    auto workspace = state_workspace_at(state, i);
    if (!workspace) continue;

    listeners_notify_layout(
      listeners,
      &runtime->layouts,
      workspace->id,
      workspace->layout_id
    );
  }

  listeners_notify_binding_mode(listeners, runtime->binding_table);
}

static void runtime_setup_bindings(runtime_t *runtime) {
  size_t bind_count = 0;
  auto bindings =
    binding_table_get_current_bindings(runtime->binding_table, &bind_count);
  if (!bindings) return;

  auto backend = runtime->backend;
  auto plan    = &runtime->plan;

  plan_reset(plan);

  auto keys = p_new(key_bind_t, bind_count);
  for (size_t i = 0; i < bind_count; ++i) {
    keys[i].keysym    = bindings[i].keysym;
    keys[i].modifiers = bindings[i].modifiers;
  }
  effect_t effect_bind_key = {
    .type        = ZDWM_EFFECT_BIND_KEY,
    .as.bind_key = {.count = bind_count, .keys = keys},
  };
  plan_push_effect(plan, &effect_bind_key);

  backend_apply_effect(backend, plan->effects, plan->count);
  plan_reset(plan);
}

static inline void runtime_setup_signal(runtime_t *runtime) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);
  sigprocmask(SIG_BLOCK, &mask, nullptr);

  runtime->signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
}

static void runtime_setup(runtime_t *runtime) {
  runtime_setup_signal(runtime);
  runtime_setup_bindings(runtime);
  runtime_init_bar(runtime);

  runtime_notify_initial_state(runtime);
}

static const layout_result_t *runtime_layout_calc(runtime_t *runtime) {
  auto result = &runtime->layout_result;
  zdwm_layout_result_reset(result);

  auto state = &runtime->state;
  for (size_t i = 0; i < state->output_count; ++i) {
    auto output = state_output_at(state, i);
    if (!output) continue;

    auto workspace = state_workspace_get(state, output->current_workspace_id);
    if (!workspace) continue;

    size_t window_count         = 0;
    size_t window_list_capacity = 0;
    window_id_t *window_ids     = nullptr;

    for (size_t j = 0; j < state_window_count(state); j++) {
      auto window = state_window_at(state, j);
      if (!window || window->workspace_id != workspace->id) continue;

      if (window_need_layout(window)) {
        window_id_t *window_id_slot =
          array_push(window_ids, window_count, window_list_capacity);
        *window_id_slot = window->id;
      } else if (window->fullscreen) {
        layout_item_t item = {
          .window_id = window->id,
          .rect      = output->geometry,
        };
        layout_result_push(result, item);
      } else if (window->maximized) {
        layout_item_t item = {
          .window_id = window->id,
          .rect      = output->workarea,
        };
        layout_result_push(result, item);
      }
    }

    auto layout_func = layout_get(&runtime->layouts, workspace->layout_id);
    if (window_count && layout_func) {
      zdwm_layout_ctx_t ctx = {
        .workspace_id      = workspace->id,
        .focused_window_id = workspace->focused_window_id,
        .output_geometry   = output->geometry,
        .workarea          = output->workarea,
        .window_ids        = window_ids,
        .window_count      = window_count,
      };
      layout_func(&ctx, result);
    }

    if (window_ids) p_delete(&window_ids);
  }

  return result->item_count ? result : nullptr;
}

static void runtime_apply_window_rect(
  state_t *state,
  window_id_t window_id,
  rect_t rect,
  plan_t *plan
) {
  auto window = state_window_get(state, window_id);
  if (!window) return;

  auto need_move   = window_need_move(window, rect.x, rect.y);
  auto need_resize = window_need_resize(window, rect.width, rect.height);

  uint32_t changed_fields = 0u;
  if (need_move) {
    changed_fields |= ZDWM_CONFIGURE_FIELD_X | ZDWM_CONFIGURE_FIELD_Y;
  }
  if (need_resize) {
    changed_fields |= ZDWM_CONFIGURE_FIELD_WIDTH | ZDWM_CONFIGURE_FIELD_HEIGHT;
    changed_fields |= ZDWM_CONFIGURE_FIELD_BORDER_WIDTH;
  }
  effect_t configure_effect = {
    .type         = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window         = window_id,
      .x              = rect.x,
      .y              = rect.y,
      .width          = rect.width - 2 * (int32_t)window->border_width,
      .height         = rect.height - 2 * (int32_t)window->border_width,
      .border_width   = window->border_width,
      .changed_fields = changed_fields,
    }
  };
  plan_push_effect(plan, &configure_effect);

  if (need_move || need_resize) {
    state_window_set_frame_rect(state, window_id, rect);
  }
}

static void runtime_arrange(runtime_t *runtime) {
  auto result = runtime_layout_calc(runtime);
  if (!result) return;

  auto state = &runtime->state;
  auto plan  = &runtime->plan;
  for (size_t i = 0; i < result->item_count; ++i) {
    auto item = &result->items[i];
    runtime_apply_window_rect(state, item->window_id, item->rect, plan);
  }
}

static policy_context_t policy_context_init(runtime_t *runtime) {
  policy_context_t ctx = {
    .interaction = &runtime->interaction,

    .bind_table = runtime->binding_table,
    .state      = &runtime->state,
    .rules      = &runtime->rules,
    .listeners  = &runtime->listeners,
    .border     = &runtime->border,
    .layouts    = &runtime->layouts,
    .bar        = {
             .visible  = &runtime->bar.visible,
             .windows  = &runtime->bar_windows,
             .height   = runtime->bar.config.height,
             .show_top = runtime->bar.config.show_top,
    },
  };

  return ctx;
}

static void runtime_scan(runtime_t *runtime) {
  auto result = backend_scan_windows(runtime->backend);
  if (!result) return;

  policy_context_t ctx = policy_context_init(runtime);
  ctx.listeners        = nullptr;

  auto backend        = runtime->backend;
  auto command_buffer = &runtime->command_buffer;
  auto plan           = &runtime->plan;

  command_buffer_reset(command_buffer);
  plan_reset(plan);

  for (size_t i = 0; i < result->count; i++) {
    event_t event = {
      .type                  = ZDWM_EVENT_WINDOW_MAP_REQUEST,
      .as.window_map_request = result->windows[i],
    };
    policy_route_event(&ctx, &event, command_buffer);
  }

  policy_apply_command(&ctx, command_buffer, plan);
  if (plan->need_relayout) runtime_arrange(runtime);
  if (plan->count) backend_apply_effect(backend, plan->effects, plan->count);

  backend_scan_result_destroy(result);
  result = nullptr;

  listeners_notify_initial_windows(&runtime->listeners, &runtime->state);
}

static void runtime_handle_event(runtime_t *runtime, short int revents) {
  auto backend        = runtime->backend;
  auto command_buffer = &runtime->command_buffer;
  auto plan           = &runtime->plan;
  auto ctx            = policy_context_init(runtime);

  if (revents & POLLHUP) {
    runtime->running = false;
  }

  if (!(revents & POLLIN)) return;

  event_t event = {0};
  while (backend_poll_event(backend, &event)) {
    command_buffer_reset(command_buffer);
    plan_reset(plan);

    policy_route_event(&ctx, &event, command_buffer);
    policy_apply_command(&ctx, command_buffer, plan);
    if (plan->need_relayout) runtime_arrange(runtime);
    if (plan->count) backend_apply_effect(backend, plan->effects, plan->count);

    if (plan->quit) {
      runtime->running      = false;
      runtime->will_restart = plan->will_restart;
    }

    event_cleanup(&event);
  }
}

static inline void runtime_update_bar(runtime_t *runtime) {
  auto bar = &runtime->bar;
  if (!bar->visible) return;

  bar_update(bar);
  if (bar_draw(bar)) backend_flush(runtime->backend);
}

static void runtime_run_event_loop(runtime_t *runtime) {
  runtime->running = true;

  auto backend = runtime->backend;

  auto backend_fd = backend_get_fd(backend);
  if (backend_fd < 0) {
    runtime->running = false;
    return;
  }

  struct pollfd fds[] = {
    [0] = {.fd = backend_fd, .events = POLLIN | POLLHUP},
    [1] = {.fd = runtime->bar.timerfd, .events = POLLIN},
    [2] = {.fd = runtime->signal_fd, .events = POLLIN},
  };

  while (runtime->running) {
    auto ready = poll(fds, countof(fds), -1);
    if (ready < 0) continue;

    runtime_handle_event(runtime, fds[0].revents);

    if (fds[1].revents & POLLIN) {
      uint64_t expirations = 0;
      read(fds[1].fd, &expirations, sizeof(expirations));
      if (expirations > 0) {
        runtime_update_bar(runtime);
      }
    }

    if (fds[2].revents & POLLIN) {
      struct signalfd_siginfo info = {0};
      read(fds[2].fd, &info, sizeof(info));
      runtime->running = false;
    }
  }
}

bool runtime_run(const char *config_so_path, const char *display_name) {
  auto backend = backend_create(nullptr);
  auto detect  = backend_detect(backend);
  if (!backend || !detect || detect->output_count == 0) {
    fatal("backend detect failed");
  }

  runtime_init_desc_t desc = {
    .backend      = backend,
    .outputs      = detect->outputs,
    .output_count = detect->output_count,
  };
  if (!runtime_config_load(nullptr, &desc)) {
    fatal("failed to build runtime inputs");
  }

  backend = nullptr;

  auto runtime = p_new(runtime_t, 1);
  bool inited  = runtime_init(runtime, &desc);

  runtime_init_desc_cleanup(&desc);
  backend_detect_destroy(detect);

  if (!inited) {
    fatal("runtime init failed");
  }

  runtime_setup(runtime);
  runtime_scan(runtime);
  runtime_run_event_loop(runtime);

  auto restart = runtime->will_restart;
  runtime_shutdown(runtime);
  p_delete(&runtime);

  return restart;
}
