#include "config/runtime_config.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "core/layout.h"
#include "core/runtime.h"
#include "helpers.h"

struct backend_t {
  int unused;
};

void backend_destroy(backend_t *backend) { free(backend); }

static backend_t *test_backend_create(void) {
  return calloc(1, sizeof(backend_t));
}

static void assert_default_layouts(const runtime_init_desc_t *desc) {
  assert(desc);
  assert(layout_registry_count(&desc->layouts) == 3);

  const layout_slot_t *tile = layout_registry_at(&desc->layouts, 0);
  const layout_slot_t *monocle = layout_registry_at(&desc->layouts, 1);
  const layout_slot_t *floating = layout_registry_at(&desc->layouts, 2);

  assert(tile);
  assert(monocle);
  assert(floating);
  assert(strcmp(tile->name, "tile") == 0);
  assert(strcmp(monocle->name, "monocle") == 0);
  assert(strcmp(floating->name, "floating") == 0);
}

static void test_runtime_config_loads_user_library(const char *fixture_path) {
  config_test_clear_env();

  output_info_t outputs[] = {
    {
      .name = "HDMI-A-1",
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
  };

  runtime_init_desc_t desc = {
    .outputs = outputs,
    .output_count = 1,
  };
  assert(runtime_config_load(fixture_path, &desc));
  assert(desc.config_module_handle != nullptr);
  assert(layout_registry_count(&desc.layouts) == 1);
  assert(desc.workspace_count == 1);
  assert(strcmp(desc.workspaces[0].name, "from-so") == 0);
  assert(desc.workspaces[0].output_index == 0);
  assert(desc.workspaces[0].layout_count == 1);
  assert(desc.workspaces[0].initial_layout_id == 0);

  const layout_slot_t *layout = layout_registry_at(&desc.layouts, 0);
  assert(layout);
  assert(strcmp(layout->name, "test-only") == 0);
  assert(strcmp(layout->symbol, "[T]") == 0);

  runtime_config_cleanup(&desc);
}

static void test_runtime_config_keeps_module_loaded_after_runtime_init(
  const char *fixture_path) {
  config_test_clear_env();

  output_info_t outputs[] = {
    {
      .name = "HDMI-A-1",
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
  };
  window_id_t window_ids[] = {42};

  runtime_init_desc_t desc = {
    .backend = test_backend_create(),
    .outputs = outputs,
    .output_count = 1,
  };
  assert(desc.backend);
  assert(runtime_config_load(fixture_path, &desc));

  runtime_t runtime = {0};
  assert(runtime_init(&runtime, &desc));
  assert(desc.config_module_handle == nullptr);
  runtime_init_desc_cleanup(&desc);

  assert(runtime.config_module_handle != nullptr);

  layout_fn layout = layout_get(&runtime.layouts, 0);
  assert(layout != nullptr);

  layout_result_t result = {0};

  layout_ctx_t ctx = {
    .workspace_id = 0,
    .focused_window_id = window_ids[0],
    .workarea = {.x = 11, .y = 22, .width = 333, .height = 444},
    .window_ids = window_ids,
    .window_count = 1,
  };
  assert(layout(&ctx, &result));
  assert(result.item_count == 1);
  assert(result.items[0].window_id == window_ids[0]);
  assert(result.items[0].rect.x == ctx.workarea.x);
  assert(result.items[0].rect.y == ctx.workarea.y);
  assert(result.items[0].rect.width == ctx.workarea.width);
  assert(result.items[0].rect.height == ctx.workarea.height);

  layout_result_cleanup(&result);
  runtime_shutdown(&runtime);
  assert(runtime.config_module_handle == nullptr);
}

static void
test_runtime_config_falls_back_to_defaults_when_implicit_library_is_missing(
  void) {
  config_test_clear_env();

  output_info_t outputs[] = {
    {
      .name = "eDP-1",
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
    {
      .name = "DP-1",
      .geometry = {.x = 1920, .y = 0, .width = 2560, .height = 1440},
    },
  };

  char temp_root[PATH_MAX] = {0};
  config_test_make_temp_dir(temp_root, sizeof(temp_root),
                            "zdwm-runtime-config-missing");
  assert(setenv("XDG_CONFIG_HOME", temp_root, 1) == 0);

  runtime_init_desc_t desc = {
    .outputs = outputs,
    .output_count = 2,
  };
  assert(runtime_config_load(nullptr, &desc));
  assert(desc.config_module_handle == nullptr);
  assert_default_layouts(&desc);
  assert(desc.workspace_count == 2);
  assert(strcmp(desc.workspaces[0].name, "main") == 0);
  assert(strcmp(desc.workspaces[1].name, "main") == 0);
  assert(desc.workspaces[0].output_index == 0);
  assert(desc.workspaces[1].output_index == 1);

  runtime_config_cleanup(&desc);
  config_test_rmdir(temp_root);
}

static void test_runtime_config_rejects_missing_explicit_library(void) {
  config_test_clear_env();

  output_info_t outputs[] = {
    {
      .name = "HDMI-A-1",
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
  };

  runtime_init_desc_t desc = {
    .outputs = outputs,
    .output_count = 1,
  };
  assert(!runtime_config_load("/tmp/definitely-missing-config.so", &desc));
  assert(layout_registry_count(&desc.layouts) == 0);
  assert(desc.workspace_count == 0);
  assert(desc.workspaces == nullptr);
  assert(desc.config_module_handle == nullptr);
}

static void test_runtime_config_rejects_library_without_setup(
  const char *fixture_path) {
  config_test_clear_env();

  output_info_t outputs[] = {
    {
      .name = "HDMI-A-1",
      .geometry = {.x = 0, .y = 0, .width = 1920, .height = 1080},
    },
  };

  runtime_init_desc_t desc = {
    .outputs = outputs,
    .output_count = 1,
  };
  assert(!runtime_config_load(fixture_path, &desc));
  assert(layout_registry_count(&desc.layouts) == 0);
  assert(desc.workspace_count == 0);
  assert(desc.workspaces == nullptr);
  assert(desc.config_module_handle == nullptr);
}

int main(int argc, char **argv) {
  assert(argc == 3);

  test_runtime_config_loads_user_library(argv[1]);
  test_runtime_config_keeps_module_loaded_after_runtime_init(argv[1]);
  test_runtime_config_falls_back_to_defaults_when_implicit_library_is_missing();
  test_runtime_config_rejects_missing_explicit_library();
  test_runtime_config_rejects_library_without_setup(argv[2]);

  return 0;
}
