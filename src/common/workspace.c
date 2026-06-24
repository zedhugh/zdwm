#include "common/workspace.h"

#include "base/memory.h"

void workspace_desc_cleanup(workspace_desc_t *workspace) {
  if (!workspace) return;

  p_delete(&workspace->name);
  p_delete(&workspace->layout_ids);
  workspace->output_index = 0;
  workspace->layout_count = 0;
  workspace->initial_layout_id = ZDWM_LAYOUT_ID_INVALID;
}

void workspace_desc_list_cleanup(workspace_desc_t **list, size_t *count) {
  if (!list || !count) return;

  for (size_t i = 0; i < *count; i++) {
    workspace_desc_cleanup(&(*list)[i]);
  }

  p_delete(list);
  *count = 0;
}

bool workspace_desc_layouts_valid(const workspace_desc_t *workspace) {
  if (!workspace || !workspace->layout_count || !workspace->layout_ids) {
    return false;
  }

  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->layout_ids[i] == workspace->initial_layout_id) return true;
  }

  return false;
}

bool workspace_desc_valid(
  const workspace_desc_t *workspace,
  size_t output_count
) {
  if (!workspace || !workspace->name) return false;
  if (workspace->output_index >= output_count) return false;

  return workspace_desc_layouts_valid(workspace);
}
