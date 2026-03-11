#pragma once

#include <stddef.h>

#include "core/types.h"

typedef struct wm_window_t {
  wm_window_id_t id;
  wm_workspace_id_t workspace_id;

  wm_window_geometry_mode_t geometry_mode;
  bool floating;
  bool sticky;
  bool urgent;
  bool fixed_size;

  /* 几何信息（均为包含边框后的外框矩形） */
  wm_rect_t float_rect; /* floating 模式下记忆的外框矩形 */
  wm_rect_t frame_rect; /* 当前外框矩形（由 layout 或 float_rect 解析） */

  /* 元数据（核心算法不依赖，仅用于规则匹配和信息展示） */
  char *title;
  char *app_id;
  char *class_name;
  char *instance_name;

  bool skip_taskbar;
} wm_window_t;

typedef struct wm_workspace_t {
  wm_workspace_id_t id;
  wm_output_id_t output_id; /* 固定归属某个输出 */
  wm_layout_id_t layout_id; /* 当前活动布局（可用布局列表中的一个） */
  wm_window_id_t focused_window_id;

  /* 可用布局列表（启动时根据配置分配，运行时固定） */
  wm_layout_id_t *available_layouts;
  size_t layout_count;

  /* 名称（核心算法不依赖，仅用于状态栏显示） */
  char *name;
} wm_workspace_t;

typedef struct wm_output_t {
  wm_output_id_t id;
  bool enabled;
  wm_rect_t geometry; /* 输出完整几何 */
  wm_rect_t workarea; /* 可用区域（排除面板等） */
  wm_workspace_id_t current_workspace_id;
} wm_output_t;

typedef struct wm_state_t {
  wm_workspace_t *workspaces;
  size_t workspace_count;
  size_t workspace_capacity;

  wm_output_t *outputs;
  size_t output_count;
  size_t output_capacity;

  wm_window_t *windows;
  size_t window_count;
  size_t window_capacity;

  wm_window_id_t *stack_order;
  size_t stack_count;
  size_t stack_capacity;

  uint64_t generation;
  bool initialized;
} wm_state_t;
