#pragma once

#include "wm_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ========== 核心实体定义 ==========

typedef struct wm_window_t {
  wm_window_id_t id;
  wm_workspace_id_t workspace_id;

  // 几何模式
  wm_window_geometry_mode_t geometry_mode;
  bool floating;
  bool sticky;
  bool urgent;

  // 几何信息
  wm_rect_t float_rect;   // floating 窗口的矩形
  wm_rect_t frame_rect;   // 最终应用的矩形（由 layout 计算或 float_rect 决定）

  // 元数据（核心算法不依赖，仅用于规则匹配和服务层展示）
  char *title;
  char *app_id;
  char *class_name;
  char *instance_name;
} wm_window_t;

typedef struct wm_workspace_t {
  wm_workspace_id_t id;
  wm_output_id_t output_id;      // 固定归属某个输出
  wm_layout_id_t layout_id;
  wm_window_id_t focused_window_id;

  // 名称（核心算法不依赖，仅用于状态栏显示）
  char *name;
} wm_workspace_t;

typedef struct wm_output_t {
  wm_output_id_t id;
  bool enabled;
  wm_rect_t geometry;     // 输出完整几何
  wm_rect_t workarea;     // 可用区域（排除面板等）
  wm_workspace_id_t current_workspace_id;
} wm_output_t;

// ========== 全局状态容器 ==========

typedef struct wm_state_t {
  // 内部实现（不透明结构体，实现文件中定义）
  wm_workspace_t *workspaces;
  size_t workspace_count;
  size_t workspace_capacity;

  wm_output_t *outputs;
  size_t output_count;
  size_t output_capacity;

  wm_window_t *windows;
  size_t window_count;
  size_t window_capacity;

  // 公开的字段
  wm_window_id_t *stack_order;
  size_t stack_count;
  size_t stack_capacity;

  uint64_t generation;
  bool initialized;
} wm_state_t;

// ========== API 函数 ==========

// 初始化/清理
void wm_state_init(wm_state_t *state,
                  size_t workspace_count,
                  size_t output_count);
void wm_state_shutdown(wm_state_t *state);

// ========== Workspace 访问 ==========

wm_workspace_t *wm_state_workspace(wm_state_t *state, wm_workspace_id_t id);
wm_workspace_t *wm_state_workspace_at(wm_state_t *state, size_t index);
size_t wm_state_workspace_count(const wm_state_t *state);
bool wm_state_workspace_valid(wm_state_t *state, wm_workspace_id_t id);

// 查询某个 output 的所有 workspace
size_t wm_state_workspace_get_by_output(const wm_state_t *state,
                                         wm_output_id_t output_id,
                                         wm_workspace_t **out_workspaces);

// ========== Output 访问 ==========

wm_output_t *wm_state_output(wm_state_t *state, wm_output_id_t id);
wm_output_t *wm_state_output_at(wm_state_t *state, size_t index);
size_t wm_state_output_count(const wm_state_t *state);
bool wm_state_output_valid(wm_state_t *state, wm_output_id_t id);

// ========== Window 操作 ==========

wm_window_t *wm_state_window_add(wm_state_t *state, wm_window_id_t id);
wm_window_t *wm_state_window_get(wm_state_t *state, wm_window_id_t id);
wm_window_t *wm_state_window_at(wm_state_t *state, size_t index);
void wm_state_window_remove(wm_state_t *state, wm_window_id_t id);
size_t wm_state_window_count(const wm_state_t *state);

// ========== 查询辅助函数 ==========

/*
 * 判断工作区是否可见（某个 output 正在显示它）
 */
bool wm_state_workspace_is_visible(const wm_state_t *state,
                                   wm_workspace_id_t workspace_id);

/*
 * 判断窗口是否可见
 * 窗口可见 iff：
 * 1. 窗口的 workspace 固定归属某个 output
 * 2. 该 output 当前正显示这个 workspace
 * 3. 窗口未最小化
 */
bool wm_state_window_should_be_visible(const wm_state_t *state,
                                       const wm_window_t *window);

/*
 * 判断窗口是否在特定 output 上可见
 */
bool wm_state_window_should_be_visible_on_output(const wm_state_t *state,
                                                 const wm_window_t *window,
                                                 const wm_output_t *output);
