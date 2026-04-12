#include "config/runtime_config.h"

#include <dlfcn.h>
#include <stddef.h>
#include <zdwm/config.h>

#include "base/array.h"
#include "base/memory.h"
#include "config/defaults.h"
#include "config/loader.h"
#include "core/layout.h"
#include "core/rules.h"
#include "core/runtime.h"
#include "core/types.h"
#include "core/wm_desc.h"

struct zdwm_config_builder_t {
  layout_registry_t layouts;
  rules_t rules;
  workspace_desc_t *workspaces;
  size_t workspace_count;
  size_t workspace_capacity;
  size_t output_count;
};

void runtime_config_cleanup(runtime_init_desc_t *desc) {
  if (!desc) return;

  if (desc->layouts.slots || desc->layouts.slot_count ||
      desc->layouts.slot_capacity) {
    layout_registry_cleanup(&desc->layouts);
  }
  workspace_desc_list_cleanup(&desc->workspaces, &desc->workspace_count);
  if (desc->config_module_handle) dlclose(desc->config_module_handle);
  desc->config_module_handle = nullptr;
}

static void config_builder_cleanup(zdwm_config_builder_t *builder) {
  if (!builder) return;

  if (builder->layouts.slots || builder->layouts.slot_count ||
      builder->layouts.slot_capacity) {
    layout_registry_cleanup(&builder->layouts);
  }
  workspace_desc_list_cleanup(&builder->workspaces, &builder->workspace_count);
  builder->workspace_capacity = 0;
  builder->output_count = 0;
}

static layout_id_t runtime_config_register_layout(
  zdwm_config_builder_t *builder, const char *name, const char *symbol,
  const char *description, layout_fn fn) {
  if (!builder || !name || !symbol) return ZDWM_LAYOUT_ID_INVALID;

  return layout_register(&builder->layouts, name, symbol, description, fn);
}

static workspace_id_t runtime_config_define_workspace(
  zdwm_config_builder_t *builder, size_t output_index, const char *name,
  const layout_id_t *layout_ids, size_t layout_count,
  layout_id_t initial_layout_id) {
  if (!builder) return ZDWM_WORKSPACE_ID_INVALID;
  if (output_index >= builder->output_count) return ZDWM_WORKSPACE_ID_INVALID;
  if (builder->workspace_count >= (size_t)ZDWM_WORKSPACE_ID_INVALID) {
    return ZDWM_WORKSPACE_ID_INVALID;
  }

  workspace_desc_t desc = {
    .output_index = output_index,
    .name = name,
    .layout_ids = layout_ids,
    .layout_count = layout_count,
    .initial_layout_id = initial_layout_id,
  };
  if (!workspace_desc_valid(&desc, builder->output_count)) {
    return ZDWM_WORKSPACE_ID_INVALID;
  }

  for (size_t i = 0; i < layout_count; i++) {
    if (!layout_slot_get(&builder->layouts, layout_ids[i])) {
      return ZDWM_WORKSPACE_ID_INVALID;
    }
  }

  workspace_id_t workspace_id = (workspace_id_t)builder->workspace_count;
  workspace_desc_t *slot = array_push(
    builder->workspaces, builder->workspace_count, builder->workspace_capacity);
  slot->output_index = output_index;
  slot->name = p_strdup(name);
  slot->layout_ids = p_copy(layout_ids, layout_count);
  slot->layout_count = layout_count;
  slot->initial_layout_id = initial_layout_id;
  return workspace_id;
}

static bool rule_match_valid(const rule_match_t *match) {
  if (!match) return false;

  return match->app_id || match->role || match->class_name ||
         match->instance_name;
}

static bool rule_action_valid(const rule_action_t *action,
                              size_t workspace_count) {
  if (!action) return false;

  if (action->workspace == ZDWM_WORKSPACE_ID_INVALID) {
    return action->switch_to_workspace || action->fullscreen ||
           action->maximize || action->floating;
  }

  return action->workspace < workspace_count;
}

static bool runtime_config_add_rule(zdwm_config_builder_t *builder,
                                    const zdwm_rule_match_t *match,
                                    const zdwm_rule_action_t *action) {
  if (!builder) return false;
  if (!rule_match_valid(match)) return false;
  if (!rule_action_valid(action, builder->workspace_count)) return false;

  rules_t *rules = &builder->rules;
  rule_item_t *rule = array_push(rules->items, rules->count, rules->capacity);

  rule->match.app_id = p_strdup_nullable(match->app_id);
  rule->match.role = p_strdup_nullable(match->role);
  rule->match.class_name = p_strdup_nullable(match->class_name);
  rule->match.instance_name = p_strdup_nullable(match->instance_name);
  rule->action = *action;

  return true;
}

static bool config_builder_finish(zdwm_config_builder_t *builder,
                                  runtime_init_desc_t *out) {
  if (!builder || !out) return false;

  if (!builder->layouts.slot_count || !builder->workspace_count) return false;

  if (!layout_registry_move(&builder->layouts, &out->layouts)) return false;
  if (!rules_move(&builder->rules, &out->rules)) return false;
  out->workspaces = builder->workspaces;
  out->workspace_count = builder->workspace_count;
  builder->workspaces = nullptr;
  builder->workspace_count = 0;
  builder->workspace_capacity = 0;
  builder->output_count = 0;
  return true;
}

static bool runtime_config_build(zdwm_config_setup_fn *setup,
                                 const output_info_t *outputs,
                                 size_t output_count,
                                 runtime_init_desc_t *out) {
  if (!setup || !out) return false;
  zdwm_config_builder_t builder = {0};
  builder.output_count = output_count;

  zdwm_api_t api = {
    .abi_version = ZDWM_CONFIG_ABI_VERSION,
    .builtin_layouts =
      {
        .tile = nullptr,
        .monocle = nullptr,
      },
    .register_layout = runtime_config_register_layout,
    .define_workspace = runtime_config_define_workspace,
    .add_rule = runtime_config_add_rule,
  };
  bool ok = setup(&api, &builder, outputs, output_count) &&
            config_builder_finish(&builder, out);
  config_builder_cleanup(&builder);
  return ok;
}

bool runtime_config_load(const char *override_path, runtime_init_desc_t *out) {
  if (!out || !out->outputs || out->output_count == 0) return false;
  runtime_config_cleanup(out);

  config_loader_t loader = {0};
  bool ok = false;
  if (!config_loader_load(&loader, override_path)) goto cleanup;

  zdwm_config_setup_fn *setup =
    loader.setup ? loader.setup : config_defaults_build;
  ok = runtime_config_build(setup, out->outputs, out->output_count, out);
  if (ok) {
    out->config_module_handle = loader.handle;
    loader.handle = nullptr;
  }

cleanup:
  config_loader_cleanup(&loader);
  return ok;
}
