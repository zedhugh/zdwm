#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/layer.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

/* 核心实体定义 */
typedef struct window_t {
  window_id_t id;
  window_id_t transient_for;
  workspace_id_t workspace_id;

  layer_type_t layer;
  window_geometry_mode_t geometry_mode;
  bool floating;
  bool sticky;
  bool urgent;
  bool fixed_size;

  /* 几何信息（均为包含边框后的外框矩形） */
  rect_t float_rect; /* floating 模式下记忆的外框矩形 */
  rect_t frame_rect; /* 当前外框矩形（由 layout 或 float_rect 解析） */

  /* 元数据（核心算法不依赖，仅用于规则匹配和信息展示） */
  char *title;
  char *app_id;
  char *role;
  char *class_name;
  char *instance_name;

  bool skip_taskbar;
} window_t;

typedef struct workspace_t {
  workspace_id_t id;
  output_id_t output_id; /* 固定归属某个输出 */
  layout_id_t layout_id; /* 当前活动布局（可用布局列表中的一个） */
  window_id_t focused_window_id;

  /* 当前 workspace 的可用布局列表 */
  const layout_id_t *available_layouts;
  size_t layout_count;

  /* 名称（核心算法不依赖） */
  const char *name;
} workspace_t;

typedef struct output_t {
  output_id_t id;
  workspace_id_t current_workspace_id;
  const char *name;
  rect_t geometry; /* 输出完整几何 */
  rect_t workarea; /* 可用区域（排除面板等） */
} output_t;

/* 全局状态容器 */
typedef struct state_t {
  /* 按 id 稳定引用的 workspace / output 集合 */
  workspace_t *workspaces;
  size_t workspace_count;

  output_t *outputs;
  size_t output_count;

  window_t *windows;
  size_t window_count;
  size_t window_capacity;

  /* 分层堆叠顺序：每层内部从低到高排列 */
  layer_stack_t stacks[ZDWM_WINDOW_LAYER_COUNT];
} state_t;

/*
 * 调用约束：
 * - state 必须是有效的非空指针
 * - state_init() / state_cleanup() 必须成对调用
 * - 同一个 state 生命周期内，state_init() 不允许重复调用
 * - 除 state_init() 外，其余 state 相关接口都要求 state 已初始化
 * - workspaces[i] 必须满足 workspace_desc_valid(&workspaces[i],
 * output_count)
 * - 传入空指针、未初始化对象或违反前置条件属于调用方错误
 *
 * 初始化时根据描述表构建 outputs[] 与 workspaces[]。
 * workspace_id 由 state 按描述表顺序分配并保持稳定，不从 desc 中读取。
 * 每个 output 的 current_workspace_id 会自动设置为首个归属到该 output 的
 * workspace；若某个 output 没有任何归属 workspace，state_init() 会失败。
 */
void state_init(state_t *state, const output_info_t *outputs,
                size_t output_count, const workspace_desc_t *workspaces,
                size_t workspace_count);
void state_cleanup(state_t *state);

/*
 * state 持有的 workspace 集合接口
 *
 * 这组接口对外只提供只读访问。
 * workspace 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
const workspace_t *state_workspace_get(const state_t *state, workspace_id_t id);
const workspace_t *state_workspace_at(const state_t *state, size_t index);
bool state_workspace_cycle_layout(state_t *state, workspace_id_t workspace_id);
bool state_workspace_set_layout_by_index(state_t *state,
                                         workspace_id_t workspace_id,
                                         size_t index);
bool state_workspace_set_layout_by_id(state_t *state,
                                      workspace_id_t workspace_id,
                                      layout_id_t layout_id);
void state_workspace_set_focused_window(state_t *state,
                                        workspace_id_t workspace_id,
                                        window_id_t window_id);
size_t state_workspace_count(const state_t *state);
bool state_workspace_valid(const state_t *state, workspace_id_t id);

/*
 * state 持有的 output 集合接口
 *
 * 这组接口对外只提供只读访问。
 * output 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
const output_t *state_output_get(const state_t *state, output_id_t id);
const output_t *state_output_at(const state_t *state, size_t index);
void state_output_set_workarea(state_t *state, output_id_t output_id,
                               rect_t workarea);
void state_output_set_current_workspace(state_t *state, output_id_t output_id,
                                        workspace_id_t workspace_id);
size_t state_output_count(const state_t *state);
bool state_output_valid(const state_t *state, output_id_t id);

/*
 * state 持有的 window 集合接口
 *
 * 这组接口负责管理 state_t.windows[] 容器本身，并维护分层的堆叠顺序。
 * 接口返回的 window 对象仅提供只读访问。
 * window 的运行期可变状态必须通过专门的 state 级更新接口修改。
 */
/*
 * 添加窗口，并将其追加到对应堆栈顶部
 *
 * info 提供窗口的基础信息
 * workspace 归属与其他策略相关字段由后续 state_window_* 接口设置。
 */
const window_t *state_window_add(state_t *state, const window_info_t *info);
const window_t *state_window_get(const state_t *state, window_id_t id);
const window_t *state_window_at(const state_t *state, size_t index);
/* 删除窗口，并同步将其从对应的堆叠栈中移除 */
void state_window_remove(state_t *state, window_id_t id);
size_t state_window_count(const state_t *state);

/* state 持有的单个 window 状态更新接口 */
void state_window_set_workspace(state_t *state, window_id_t window_id,
                                workspace_id_t workspace_id);
void state_window_set_geometry_mode(state_t *state, window_id_t window_id,
                                    window_geometry_mode_t geometry_mode);
void state_window_set_floating(state_t *state, window_id_t window_id,
                               bool floating);
void state_window_set_sticky(state_t *state, window_id_t window_id,
                             bool sticky);
void state_window_set_urgent(state_t *state, window_id_t window_id,
                             bool urgent);
void state_window_set_fixed_size(state_t *state, window_id_t window_id,
                                 bool fixed_size);
void state_window_set_skip_taskbar(state_t *state, window_id_t window_id,
                                   bool skip_taskbar);
void state_window_set_float_rect(state_t *state, window_id_t window_id,
                                 rect_t float_rect);
void state_window_set_frame_rect(state_t *state, window_id_t window_id,
                                 rect_t frame_rect);
/**
 * @brief 按字段掩码转移窗口元数据的所有权
 * @param state 状态实例指针
 * @param window_id 目标窗口 id
 * @param metadata 元数据来源；成功时仅被转移字段会被置空
 * @param changed_fields 需要转移的字段掩码
 * @return 成功返回 true；若目标窗口不存在则返回 false
 * @details
 * 成功时，仅转移 changed_fields 指定的字段到目标 window，并将 metadata 中对应
 * 字段置空；未置位字段保持在 metadata 中，所有权仍归调用方。
 *
 * 失败时，不接管 metadata 中任何字段的所有权，metadata 保持不变，仍由调用方负
 * 责后续访问与释放。
 */
bool state_window_take_metadata(state_t *state, window_id_t window_id,
                                window_metadata_t *metadata,
                                uint32_t changed_fields);
bool state_window_set_title(state_t *state, window_id_t window_id,
                            const char *title);
bool state_window_set_app_id(state_t *state, window_id_t window_id,
                             const char *app_id);
bool state_window_set_role(state_t *state, window_id_t window_id,
                           const char *role);
bool state_window_set_class(state_t *state, window_id_t window_id,
                            const char *class_name);
bool state_window_set_instance(state_t *state, window_id_t window_id,
                               const char *instance_name);

bool state_stack_raise(state_t *state, window_id_t window_id);
bool state_stack_lower(state_t *state, window_id_t window_id);
