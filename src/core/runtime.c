#include "core/runtime.h"

#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/layout.h>

#include "action.h"
#include "base/array.h"
#include "base/memory.h"
#include "core/backend.h"
#include "core/binding.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/layout.h"
#include "core/plan.h"
#include "core/policy.h"
#include "core/rules.h"
#include "core/state.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

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

bool runtime_init(runtime_t *runtime, runtime_init_desc_t *desc) {
  if (!runtime || !runtime_init_desc_valid(desc)) return false;

  p_clear(runtime, 1);
  runtime->backend = desc->backend;
  layout_registry_move(&desc->layouts, &runtime->layouts);
  rules_move(&desc->rules, &runtime->rules);
  runtime->config_module_handle = desc->config_module_handle;
  runtime->binding_table        = desc->binding_table;
  desc->backend                 = nullptr;
  desc->config_module_handle    = nullptr;
  desc->binding_table           = nullptr;

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

void runtime_init_desc_cleanup(runtime_init_desc_t *desc) {
  if (!desc) return;

  binding_table_destroy(desc->binding_table);
  desc->binding_table = nullptr;

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

void runtime_shutdown(runtime_t *runtime) {
  runtime->running = false;
  plan_cleanup(&runtime->plan);
  command_buffer_cleanup(&runtime->command_buffer);
  layout_registry_cleanup(&runtime->layouts);
  rules_cleanup(&runtime->rules);
  state_cleanup(&runtime->state);
  layout_result_cleanup(&runtime->layout_result);
  binding_table_destroy(runtime->binding_table);
  runtime->binding_table = nullptr;
  backend_destroy(runtime->backend);
  runtime->backend = nullptr;
  if (runtime->config_module_handle) dlclose(runtime->config_module_handle);
  runtime->config_module_handle = nullptr;
}

void runtime_setup(runtime_t *runtime) {
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

static const layout_result_t *runtime_layout_calc(runtime_t *runtime) {
  auto result = &runtime->layout_result;
  zdwm_layout_result_reset(result);

  auto state = &runtime->state;
  for (size_t i = 0; i < state->output_count; ++i) {
    auto output = state_output_at(state, i);
    if (!output) continue;

    auto workspace = state_workspace_get(state, output->current_workspace_id);
    if (!workspace) continue;

    auto layout_slot = layout_slot_get(&runtime->layouts, workspace->layout_id);
    if (!layout_slot || !layout_slot->fn) continue;

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
      } else if (window->geometry_mode == ZDWM_GEOMETRY_FULLSCREEN) {
        layout_item_t item = {
          .window_id = window->id,
          .rect      = output->geometry,
        };
        layout_result_push(result, item);
      } else if (window->geometry_mode == ZDWM_GEOMETRY_MAXIMIZED) {
        layout_item_t item = {
          .window_id = window->id,
          .rect      = output->workarea,
        };
        layout_result_push(result, item);
      }
    }

    if (window_count) {
      zdwm_layout_ctx_t ctx = {
        .workspace_id      = workspace->id,
        .focused_window_id = workspace->focused_window_id,
        .output_geometry   = output->geometry,
        .workarea          = output->workarea,
        .window_ids        = window_ids,
        .window_count      = window_count,
      };
      layout_slot->fn(&ctx, result);
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
  }
  effect_t configure_effect = {
    .type         = ZDWM_EFFECT_CONFIGURE_WINDOW,
    .as.configure = {
      .window         = window_id,
      .x              = rect.x,
      .y              = rect.y,
      .width          = rect.width,
      .height         = rect.height,
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

void runtime_run(runtime_t *runtime) {
  runtime->running = true;

  backend_t *backend               = runtime->backend;
  command_buffer_t *command_buffer = &runtime->command_buffer;
  plan_t *plan                     = &runtime->plan;

  policy_context_t ctx = {
    .bind_table = runtime->binding_table,
    .state      = &runtime->state,
    .rules      = &runtime->rules,
    .action_ctx = {
      .spawn = spawn,
    },
  };

  while (runtime->running) {
    event_t event = {0};
    if (!backend_next_event(backend, &event)) break;

    command_buffer_reset(command_buffer);
    plan_reset(plan);

    policy_route_event(&ctx, &event, command_buffer);
    policy_apply_command(&ctx, command_buffer, plan);
    if (plan->need_relayout) runtime_arrange(runtime);
    if (plan->count) backend_apply_effect(backend, plan->effects, plan->count);

    event_cleanup(&event);
  }
}
