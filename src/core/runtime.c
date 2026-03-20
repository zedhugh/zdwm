#include "core/runtime.h"

#include <stddef.h>

#include "core/backend.h"
#include "core/state.h"
#include "core/types.h"
#include "core/wm_desc.h"
#include "utils.h"

bool wm_runtime_init(wm_runtime_t *runtime) {
  p_clear(runtime, 1);

  wm_backend_t *backend = wm_backend_create(nullptr);
  if (!backend) return false;

  runtime->backend = backend;

  wm_backend_detect_t *detect = wm_backend_detect(backend);
  if (!detect || detect->output_count <= 0) {
    wm_backend_destroy(backend);
    return false;
  }

  wm_workspace_desc_t *workspaces =
    p_new(wm_workspace_desc_t, detect->output_count);
  static wm_layout_id_t ids[] = {0, 1, 2};
  for (size_t i = 0; i < detect->output_count; i++) {
    wm_workspace_desc_t *desc = &workspaces[i];
    desc->output_index = i;
    desc->name = "web";
    desc->layout_ids = ids;
    desc->layout_count = countof(ids);
    desc->initial_layout_id = 1;
  }

  wm_state_init(&runtime->state, detect->outputs, detect->output_count,
                workspaces, detect->output_count);
  p_delete(&workspaces);
  wm_backend_detect_destroy(detect);
  detect = nullptr;

  return true;
}

void wm_runtime_shutdown(wm_runtime_t *runtime) {
  runtime->running = false;
  wm_state_cleanup(&runtime->state);
  wm_backend_destroy(runtime->backend);
  runtime->backend = nullptr;
}
