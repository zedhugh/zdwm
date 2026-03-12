#pragma once

#include <stddef.h>

#include "core/types.h"

typedef struct wm_output_info_t {
  const char *name;
  wm_rect_t geometry;
} wm_output_info_t;

/*
 * 由 backend 提供的窗口基础信息。
 *
 * 这些字符串由调用方提供；state 如需长期持有，应自行复制。
 */
typedef struct wm_window_info_t {
  wm_window_id_t id;
  wm_rect_t frame_rect;

  const char *title;
  const char *app_id;
  const char *class_name;
  const char *instance_name;

  wm_window_geometry_mode_t geometry_mode;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;
} wm_window_info_t;

typedef struct wm_workspace_desc_t {
  wm_output_id_t output_id;
  const char *name;
  const wm_layout_id_t *layout_ids;
  size_t layout_count;
  wm_layout_id_t initial_layout_id;
} wm_workspace_desc_t;
