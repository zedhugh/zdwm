#pragma once

#include <stddef.h>
#include <zdwm/listeners.h>

#include "common/listeners.h"
#include "interface/types.h"

/**
 * @brief 通知当前 output 信息
 *
 * @param listeners 通知函数集合
 * @param output_id 当前 output id
 */
void listeners_notify_current_output(
  const listeners_t *listeners,
  output_id_t output_id
);

/**
 * @brief 通知初始化 workspace 列表
 */
void listeners_notify_initial_workspaces(
  const listeners_t *listeners,
  const state_t *state
);

/**
 * @brief 通知当前激活的 workspace 信息
 */
void listeners_notify_workspace_active(
  const listeners_t *listeners,
  output_id_t output_id,
  workspace_id_t workspace_id
);

/**
 * @brief 通知当前布局信息
 *
 * @param listeners     通知函数集合
 * @param layouts       布局注册系统
 * @param workspace_id  当前 workspace id
 * @param layout_id     当前 workspace 的当前布局 id
 */
void listeners_notify_layout(
  const listeners_t *listeners,
  const layout_registry_t *layouts,
  workspace_id_t workspace_id,
  layout_id_t layout_id
);

/**
 * @brief 通知当前绑定模式信息
 */
void listeners_notify_binding_mode(
  const listeners_t *listeners,
  const binding_table_t *binding_table
);

/**
 * @brief 通知初始窗口列表
 */
void listeners_notify_initial_windows(
  const listeners_t *listeners,
  const state_t *state
);

/**
 * @brief 通知新增窗口
 */
void listeners_notify_window_added(
  const listeners_t *listeners,
  const state_t *state,
  window_id_t window_id
);

/**
 * @brief 通知窗口信息更新
 */
void listeners_notify_window_updated(
  const listeners_t *listeners,
  const state_t *state,
  window_id_t window_id
);

/**
 * @brief 通知移除窗口
 */
void listeners_notify_window_removed(
  const listeners_t *listeners,
  window_id_t window_id
);
