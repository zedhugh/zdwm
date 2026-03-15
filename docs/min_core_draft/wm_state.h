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

  // 几何信息（均为包含边框后的外框矩形）
  wm_rect_t float_rect;   // floating 模式下记忆的外框矩形
  wm_rect_t frame_rect;   // 当前最终外框矩形（由 layout 或 float_rect 解析）

  // 元数据（核心算法不依赖，仅用于规则匹配和服务层展示）
  char *title;
  char *app_id;
  char *class_name;
  char *instance_name;

  // 任务栏可见性提示
  bool skip_taskbar;
} wm_window_t;

typedef struct wm_workspace_t {
  wm_workspace_id_t id;
  wm_output_id_t output_id;      // 固定归属某个输出
  wm_layout_id_t layout_id;      // 当前活动布局（可用布局列表中的一个）
  wm_window_id_t focused_window_id;

  // 可用布局列表（启动时根据配置分配，之后固定不变）
  const wm_layout_id_t *available_layouts;
  size_t layout_count;

  // 名称（核心算法不依赖，仅用于状态栏显示）
  char *name;
} wm_workspace_t;

typedef struct wm_output_t {
  wm_output_id_t id;
  wm_rect_t geometry;     // 输出完整几何（bootstrap 后固定）
  wm_rect_t workarea;     // 可用区域（排除面板等，bootstrap 后固定）
  wm_workspace_id_t current_workspace_id;
} wm_output_t;

// ========== 全局状态容器 ==========

typedef struct wm_state_t {
  // 头文件级草案当前直接公开字段，便于讨论；实现阶段可再封装
  wm_workspace_t *workspaces;
  size_t workspace_count;

  wm_output_t *outputs;
  size_t output_count;

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
// workspace/output 集合大小在 init 时一次性确定，之后不再变化。
void wm_state_init(wm_state_t *state,
                  size_t workspace_count,
                  size_t output_count);
void wm_state_shutdown(wm_state_t *state);

// ========== Workspace 访问 ==========

wm_workspace_t *wm_state_workspace(wm_state_t *state, wm_workspace_id_t id);
wm_workspace_t *wm_state_workspace_at(wm_state_t *state, size_t index);
size_t wm_state_workspace_count(const wm_state_t *state);
bool wm_state_workspace_valid(wm_state_t *state, wm_workspace_id_t id);
void wm_state_workspace_set_focused_window(wm_state_t *state,
                                           wm_workspace_id_t workspace_id,
                                           wm_window_id_t window_id);

// 查询某个 output 的所有 workspace（只读视图）
size_t wm_state_workspace_get_by_output(const wm_state_t *state,
                                         wm_output_id_t output_id,
                                         wm_workspace_t **out_workspaces);

// ========== Workspace 布局操作 ==========

/*
 * 设置 workspace 的可用布局列表。
 * 仅允许在 bootstrap / 初始化阶段调用；运行时不应修改。
 */
void wm_workspace_set_layouts(wm_workspace_t *workspace,
                              const wm_layout_id_t *layouts,
                              size_t count);

/*
 * 获取 workspace 的可用布局列表。
 * 该列表在 bootstrap 完成后保持不变。
 */
const wm_layout_id_t *wm_workspace_get_layouts(const wm_workspace_t *workspace,
                                                size_t *out_count);

/*
 * 切换到下一个布局（循环）
 */
bool wm_workspace_cycle_layout(wm_workspace_t *workspace);

/*
 * 切换到指定索引的布局
 */
bool wm_workspace_set_layout_by_index(wm_workspace_t *workspace, size_t index);

/*
 * 切换到指定 ID 的布局（如果该布局在可用列表中）
 */
bool wm_workspace_set_layout_by_id(wm_workspace_t *workspace, wm_layout_id_t id);

// ========== Output 访问 ==========

wm_output_t *wm_state_output(wm_state_t *state, wm_output_id_t id);
wm_output_t *wm_state_output_at(wm_state_t *state, size_t index);
size_t wm_state_output_count(const wm_state_t *state);
bool wm_state_output_valid(wm_state_t *state, wm_output_id_t id);
/*
 * 切换某个 output 当前显示的 workspace。
 * 这不会修改 output 集合本身，也不会改变 workspace->output_id 绑定。
 * 调用方必须保证目标 workspace 固定归属于该 output。
 */
void wm_state_output_set_current_workspace(wm_state_t *state,
                                           wm_output_id_t output_id,
                                           wm_workspace_id_t workspace_id);

// ========== Window 操作 ==========

wm_window_t *wm_state_window_add(wm_state_t *state, wm_window_id_t id);
wm_window_t *wm_state_window_get(wm_state_t *state, wm_window_id_t id);
wm_window_t *wm_state_window_at(wm_state_t *state, size_t index);
void wm_state_window_remove(wm_state_t *state, wm_window_id_t id);
size_t wm_state_window_count(const wm_state_t *state);
void wm_state_window_set_workspace(wm_state_t *state, wm_window_id_t window_id,
                                   wm_workspace_id_t workspace_id);
void wm_state_window_set_geometry_mode(wm_state_t *state,
                                       wm_window_id_t window_id,
                                       wm_window_geometry_mode_t geometry_mode);
void wm_state_window_set_floating(wm_state_t *state, wm_window_id_t window_id,
                                  bool floating);
void wm_state_window_set_sticky(wm_state_t *state, wm_window_id_t window_id,
                                bool sticky);
void wm_state_window_set_urgent(wm_state_t *state, wm_window_id_t window_id,
                                bool urgent);
void wm_state_window_set_skip_taskbar(wm_state_t *state,
                                      wm_window_id_t window_id,
                                      bool skip_taskbar);
void wm_state_window_set_float_rect(wm_state_t *state, wm_window_id_t window_id,
                                    wm_rect_t float_rect);
void wm_state_window_set_frame_rect(wm_state_t *state, wm_window_id_t window_id,
                                    wm_rect_t frame_rect);
bool wm_state_window_set_title(wm_state_t *state, wm_window_id_t window_id,
                               const char *title);
bool wm_state_window_set_app_id(wm_state_t *state, wm_window_id_t window_id,
                                const char *app_id);
bool wm_state_window_set_class(wm_state_t *state, wm_window_id_t window_id,
                               const char *class_name);
bool wm_state_window_set_instance(wm_state_t *state, wm_window_id_t window_id,
                                  const char *instance_name);

// ========== 查询辅助函数 ==========

/*
 * 判断工作区是否可见（某个 output 正在显示它）
 */
bool wm_state_workspace_is_visible(const wm_state_t *state,
                                   wm_workspace_id_t workspace_id);

/*
 * 判断窗口是否可见
 * 窗口可见 iff：
 * 1. 若窗口是 sticky，则只要求窗口未最小化
 * 2. 否则，窗口的 workspace 固定归属某个 output
 * 3. 且该 output 当前正显示这个 workspace
 */
bool wm_state_window_should_be_visible(const wm_state_t *state,
                                       const wm_window_t *window);

/*
 * 判断窗口是否在特定 output 上可见
 */
bool wm_state_window_should_be_visible_on_output(const wm_state_t *state,
                                                 const wm_window_t *window,
                                                 const wm_output_t *output);
