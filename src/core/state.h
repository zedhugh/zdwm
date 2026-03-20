#pragma once

#include <stddef.h>

#include "core/types.h"
#include "core/wm_desc.h"

/* 核心实体定义 */
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
  const char *title;
  const char *app_id;
  const char *class_name;
  const char *instance_name;

  bool skip_taskbar;
} wm_window_t;

typedef struct wm_workspace_t {
  wm_workspace_id_t id;
  wm_output_id_t output_id; /* 固定归属某个输出 */
  wm_layout_id_t layout_id; /* 当前活动布局（可用布局列表中的一个） */
  wm_window_id_t focused_window_id;

  /* 当前 workspace 的可用布局列表 */
  const wm_layout_id_t *available_layouts;
  size_t layout_count;

  /* 名称（核心算法不依赖） */
  const char *name;
} wm_workspace_t;

typedef struct wm_output_t {
  wm_output_id_t id;
  wm_workspace_id_t current_workspace_id;
  const char *name;
  wm_rect_t geometry; /* 输出完整几何 */
  wm_rect_t workarea; /* 可用区域（排除面板等） */
} wm_output_t;

/* 全局状态容器 */
typedef struct wm_state_t {
  /* 按 id 稳定引用的 workspace / output 集合 */
  wm_workspace_t *workspaces;
  size_t workspace_count;

  wm_output_t *outputs;
  size_t output_count;

  wm_window_t *windows;
  size_t window_count;
  size_t window_capacity;

  /* 与 windows[] 一一对应的堆叠顺序视图，长度等于 window_count */
  wm_window_id_t *stack_order;

} wm_state_t;

/*
 * 调用约束：
 * - state 必须是有效的非空指针
 * - wm_state_init() / wm_state_cleanup() 必须成对调用
 * - 同一个 state 生命周期内，wm_state_init() 不允许重复调用
 * - 除 wm_state_init() 外，其余 state 相关接口都要求 state 已初始化
 * - workspaces[i] 必须满足 wm_workspace_desc_valid(&workspaces[i],
 * output_count)
 * - 传入空指针、未初始化对象或违反前置条件属于调用方错误
 *
 * 初始化时根据描述表构建 outputs[] 与 workspaces[]。
 * workspace_id 由 state 按描述表顺序分配并保持稳定，不从 desc 中读取。
 * 每个 output 的 current_workspace_id 会自动设置为首个归属到该 output 的
 * workspace；若某个 output 没有任何归属 workspace，wm_state_init() 会失败。
 */
void wm_state_init(wm_state_t *state, const wm_output_info_t *outputs,
                   size_t output_count, const wm_workspace_desc_t *workspaces,
                   size_t workspace_count);
void wm_state_cleanup(wm_state_t *state);

/*
 * state 持有的 workspace 集合接口
 *
 * 这组接口对外只提供只读访问。
 * workspace 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
const wm_workspace_t *wm_state_workspace_get(const wm_state_t *state,
                                             wm_workspace_id_t id);
const wm_workspace_t *wm_state_workspace_at(const wm_state_t *state,
                                            size_t index);
bool wm_state_workspace_cycle_layout(wm_state_t *state,
                                     wm_workspace_id_t workspace_id);
bool wm_state_workspace_set_layout_by_index(wm_state_t *state,
                                            wm_workspace_id_t workspace_id,
                                            size_t index);
bool wm_state_workspace_set_layout_by_id(wm_state_t *state,
                                         wm_workspace_id_t workspace_id,
                                         wm_layout_id_t layout_id);
void wm_state_workspace_set_focused_window(wm_state_t *state,
                                           wm_workspace_id_t workspace_id,
                                           wm_window_id_t window_id);
size_t wm_state_workspace_count(const wm_state_t *state);
bool wm_state_workspace_valid(const wm_state_t *state, wm_workspace_id_t id);

/*
 * state 持有的 output 集合接口
 *
 * 这组接口对外只提供只读访问。
 * output 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
const wm_output_t *wm_state_output_get(const wm_state_t *state,
                                       wm_output_id_t id);
const wm_output_t *wm_state_output_at(const wm_state_t *state, size_t index);
void wm_state_output_set_workarea(wm_state_t *state, wm_output_id_t output_id,
                                  wm_rect_t workarea);
void wm_state_output_set_current_workspace(wm_state_t *state,
                                           wm_output_id_t output_id,
                                           wm_workspace_id_t workspace_id);
size_t wm_state_output_count(const wm_state_t *state);
bool wm_state_output_valid(const wm_state_t *state, wm_output_id_t id);

/*
 * state 持有的 window 集合接口
 *
 * 这组接口负责管理 wm_state_t.windows[] 容器本身，并保持
 * stack_order[] 与 windows[] 同步。
 * 对外只提供只读访问。
 * window 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
/*
 * 添加窗口，并将其追加到 stack_order[] 顶部。
 *
 * info 提供 backend 已探测到的窗口基础信息。
 * workspace 归属与其他策略相关字段由后续 wm_state_window_* 接口设置。
 */
const wm_window_t *wm_state_window_add(wm_state_t *state,
                                       const wm_window_info_t *info);
const wm_window_t *wm_state_window_get(const wm_state_t *state,
                                       wm_window_id_t id);
const wm_window_t *wm_state_window_at(const wm_state_t *state, size_t index);
/* 删除窗口，并同步从 stack_order[] 中移除。 */
void wm_state_window_remove(wm_state_t *state, wm_window_id_t id);
size_t wm_state_window_count(const wm_state_t *state);

/* state 持有的单个 window 状态更新接口 */
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
void wm_state_window_set_fixed_size(wm_state_t *state, wm_window_id_t window_id,
                                    bool fixed_size);
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

/*
 * state 持有的堆叠顺序接口
 *
 * stack_order[] 是全局 z-order 的唯一真相，从下到上排列。
 * 其长度恒等于 wm_state_window_count(state)，因此不提供单独的 count 接口。
 */
const wm_window_id_t *wm_state_stack_order(const wm_state_t *state);
wm_window_id_t wm_state_stack_at(const wm_state_t *state, size_t index);
bool wm_state_stack_raise(wm_state_t *state, wm_window_id_t window_id);
bool wm_state_stack_lower(wm_state_t *state, wm_window_id_t window_id);
