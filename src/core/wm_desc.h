#pragma once

#include <stddef.h>

#include "base/memory.h"
#include "core/layer.h"
#include "core/types.h"

/*
 * 由 backend 提供的窗口基础信息。
 *
 * 这些字符串由调用方提供；state 如需长期持有，应自行复制。
 */
typedef struct window_info_t {
  window_id_t id;
  rect_t frame_rect;

  const char *title;
  const char *app_id;
  const char *class_name;
  const char *instance_name;

  window_geometry_mode_t geometry_mode;
  layer_type_t layer_type;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;
} window_info_t;

typedef struct workspace_desc_t {
  size_t output_index; /* 对应 state_init() 中 outputs[] 的索引 */
  const char *name;
  const layout_id_t *layout_ids;
  size_t layout_count;
  layout_id_t initial_layout_id;
} workspace_desc_t;

static inline void workspace_desc_cleanup(workspace_desc_t *workspace) {
  if (!workspace) return;

  p_delete(&workspace->name);
  p_delete(&workspace->layout_ids);
  workspace->output_index = 0;
  workspace->layout_count = 0;
  workspace->initial_layout_id = ZDWM_LAYOUT_ID_INVALID;
}

static inline void workspace_desc_list_cleanup(workspace_desc_t **list,
                                               size_t *count) {
  if (!list || !count) return;

  for (size_t i = 0; i < *count; i++) {
    workspace_desc_cleanup(&(*list)[i]);
  }

  p_delete(list);
  *count = 0;
}

/*
 * workspace 描述校验接口。
 *
 * 这组接口只校验描述表自身的一致性，不访问 state。
 */
static inline bool workspace_desc_layouts_valid(
  const workspace_desc_t *workspace) {
  if (!workspace || !workspace->layout_count || !workspace->layout_ids) {
    return false;
  }

  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->layout_ids[i] == workspace->initial_layout_id) return true;
  }

  return false;
}

static inline bool workspace_desc_valid(const workspace_desc_t *workspace,
                                        size_t output_count) {
  if (!workspace || !workspace->name) return false;
  if (workspace->output_index >= output_count) return false;

  return workspace_desc_layouts_valid(workspace);
}
