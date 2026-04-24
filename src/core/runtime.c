#include "core/runtime.h"

#include <dlfcn.h>

#include "action.h"
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
  binding_table_destroy(runtime->binding_table);
  runtime->binding_table = nullptr;
  backend_destroy(runtime->backend);
  runtime->backend = nullptr;
  if (runtime->config_module_handle) dlclose(runtime->config_module_handle);
  runtime->config_module_handle = nullptr;
}

void runtime_run(runtime_t *runtime) {
  runtime->running = true;

  backend_t *backend               = runtime->backend;
  command_buffer_t *command_buffer = &runtime->command_buffer;
  plan_t *plan                     = &runtime->plan;

  policy_context_t ctx = {
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
    if (plan->count) backend_apply_effect(backend, plan->effects, plan->count);

    event_cleanup(&event);
  }
}
