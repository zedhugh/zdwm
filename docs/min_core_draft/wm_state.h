#pragma once

#include "wm_types.h"

typedef struct wm_window_t {
  wm_window_id_t id;
  wm_workspace_id_t workspace_id;

  /*
   * 这里只保留控制状态，不存标题、类名、实例名等展示元数据。
   */
  wm_window_geometry_mode_t geometry_mode;

  /*
   * false: 由当前 workspace 的 layout 决定基础几何
   * true : 使用 float_rect 作为基础几何
   */
  bool floating;

  /*
   * sticky 只影响可见性，不改变 workspace 归属。
   */
  bool sticky;
  bool urgent;

  /*
   * floating 模式的基准矩形。退出 maximize/fullscreen 后可以回到这里。
   */
  wm_rect_t float_rect;

  /*
   * 最近一次实际提交给后端的矩形。
   */
  wm_rect_t frame_rect;
} wm_window_t;

typedef struct wm_workspace_t {
  wm_workspace_id_t id;

  /*
   * 这里只保留运行状态，不存名称、bar 符号等描述信息。
   */
  wm_layout_id_t layout_id;
  wm_window_id_t focused_window_id;
} wm_workspace_t;

typedef struct wm_output_t {
  wm_output_id_t id;
  wm_rect_t geometry;
  wm_rect_t workarea;
  wm_workspace_id_t current_workspace_id;
} wm_output_t;

typedef struct wm_state_t {
  wm_window_t *windows;
  size_t window_count;

  /*
   * 全局 z-order，从下到上。每个受管窗口最多出现一次。
   */
  wm_window_id_t *stack_order;
  size_t stack_count;

  wm_workspace_t *workspaces;
  size_t workspace_count;

  wm_output_t *outputs;
  size_t output_count;

  /*
   * 成功提交一次状态变更后递增。
   */
  uint64_t generation;
} wm_state_t;

void wm_state_init(wm_state_t *state);
void wm_state_shutdown(wm_state_t *state);

wm_window_t *wm_state_find_window(wm_state_t *state, wm_window_id_t id);
const wm_window_t *wm_state_find_window_const(const wm_state_t *state,
                                              wm_window_id_t id);

wm_workspace_t *wm_state_find_workspace(wm_state_t *state,
                                        wm_workspace_id_t id);
const wm_workspace_t *wm_state_find_workspace_const(const wm_state_t *state,
                                                    wm_workspace_id_t id);

wm_output_t *wm_state_find_output(wm_state_t *state, wm_output_id_t id);
const wm_output_t *wm_state_find_output_const(const wm_state_t *state,
                                              wm_output_id_t id);

const wm_output_t *wm_state_find_output_by_point(const wm_state_t *state,
                                                 wm_point_t point);

bool wm_state_workspace_is_visible(const wm_state_t *state,
                                   wm_workspace_id_t workspace_id);

bool wm_state_window_should_be_visible(const wm_state_t *state,
                                       const wm_window_t *window);

bool wm_state_window_should_be_visible_on_output(const wm_state_t *state,
                                                 const wm_window_t *window,
                                                 const wm_output_t *output);
