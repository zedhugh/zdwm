#pragma once

#include <stddef.h>

#include "core/backend.h"
#include "core/layout.h"
#include "core/state.h"

typedef struct runtime_init_desc_t {
  backend_t *backend;
  output_info_t *outputs;
  size_t output_count;

  layout_registry_t layouts;
  workspace_desc_t *workspaces;
  size_t workspace_count;
  void *config_module_handle;
} runtime_init_desc_t;

typedef struct runtime_t {
  bool running;
  bool will_restart;

  state_t state;
  layout_registry_t layouts;
  backend_t *backend;
  void *config_module_handle;
} runtime_t;

bool runtime_init(runtime_t *runtime, runtime_init_desc_t *desc);
void runtime_init_desc_cleanup(runtime_init_desc_t *desc);
void runtime_shutdown(runtime_t *runtime);
