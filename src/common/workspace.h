#pragma once

#include <stddef.h>

#include "interface/types.h"

typedef struct workspace_desc_t {
  size_t output_index; /* 对应 state_init() 中 outputs[] 的索引 */
  const char *name;
  const layout_id_t *layout_ids;
  size_t layout_count;
  layout_id_t initial_layout_id;
} workspace_desc_t;

void workspace_desc_cleanup(workspace_desc_t *workspace);
void workspace_desc_list_cleanup(workspace_desc_t **list, size_t *count);
bool workspace_desc_layouts_valid(const workspace_desc_t *workspace);
bool workspace_desc_valid(
  const workspace_desc_t *workspace,
  size_t output_count
);
