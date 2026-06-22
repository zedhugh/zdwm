#pragma once

#include <stddef.h>
#include <stdint.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "core/backend.h"
#include "core/binding.h"
#include "core/layout.h"
#include "core/listeners.h"
#include "core/rules.h"
#include "core/types.h"
#include "core/wm_desc.h"

typedef struct runtime_init_desc_t {
  backend_t *backend;
  zdwm_output_info_t *outputs;
  size_t output_count;

  layout_registry_t layouts;
  rules_t rules;
  border_config_t border;
  uint32_t fps;
  workspace_desc_t *workspaces;
  size_t workspace_count;
  void *config_module_handle;
  binding_table_t *binding_table;
  listeners_t listeners;
  zdwm_bar_config_t bar;
} runtime_init_desc_t;

bool runtime_config_load(const char *override_path, runtime_init_desc_t *out);
void runtime_config_cleanup(runtime_init_desc_t *desc);
