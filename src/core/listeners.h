#pragma once

#include <stddef.h>
#include <zdwm/listeners.h>

#include "core/types.h"

typedef struct binding_table_t binding_table_t;
typedef struct layout_registry_t layout_registry_t;
typedef struct state_t state_t;

#define ITEM(NAME, FN_TYPE)               \
  typedef struct NAME##_listener_item_t { \
    FN_TYPE *fn;                          \
    void *user_data;                      \
  } NAME##_listener_item_t

#define LISTENERS(NAME, FN_TYPE)      \
  ITEM(NAME, FN_TYPE);                \
  typedef struct NAME##_listeners_t { \
    NAME##_listener_item_t *items;    \
    size_t count;                     \
    size_t capacity;                  \
  } NAME##_listeners_t

LISTENERS(output, zdwm_current_output_id_listener);
LISTENERS(initial_workspace_list, zdwm_initial_workspace_list);
LISTENERS(workspace_active, zdwm_workspace_active_updated);
LISTENERS(layout, zdwm_layout_notify);
LISTENERS(binding_mode, zdwm_binding_mode_notify);
LISTENERS(initial_window_list, zdwm_initial_window_list);
LISTENERS(window_added, zdwm_window_added);
LISTENERS(window_updated, zdwm_window_updated);
LISTENERS(window_removed, zdwm_window_removed);

#undef ITEM
#undef LISTENERS

typedef struct listeners_t {
  output_listeners_t output_listeners;
  initial_workspace_list_listeners_t initial_workspace_listeners;
  workspace_active_listeners_t active_workspace_listeners;
  layout_listeners_t layout_listeners;
  binding_mode_listeners_t binding_mode_listeners;
  initial_window_list_listeners_t initial_window_listeners;
  window_added_listeners_t window_added_listeners;
  window_updated_listeners_t window_updated_listeners;
  window_removed_listeners_t window_removed_listeners;
} listeners_t;

void listeners_add_output_listener(
  listeners_t *listeners,
  zdwm_current_output_id_listener fn,
  void *user_data
);
void listeners_add_initial_workspace_listener(
  listeners_t *listeners,
  zdwm_initial_workspace_list fn,
  void *user_data
);
void listeners_add_active_workspace_listener(
  listeners_t *listeners,
  zdwm_workspace_active_updated fn,
  void *user_data
);
void listeners_add_layout_notify(
  listeners_t *listeners,
  zdwm_layout_notify fn,
  void *user_data
);
void listeners_add_binding_mode_notify(
  listeners_t *listeners,
  zdwm_binding_mode_notify fn,
  void *user_data
);
void listeners_add_initial_window_listener(
  listeners_t *listeners,
  zdwm_initial_window_list fn,
  void *user_data
);
void listeners_add_window_added_listener(
  listeners_t *listeners,
  zdwm_window_added fn,
  void *user_data
);
void listeners_add_window_updated_listener(
  listeners_t *listeners,
  zdwm_window_updated fn,
  void *user_data
);
void listeners_add_window_removed_listener(
  listeners_t *listeners,
  zdwm_window_removed fn,
  void *user_data
);

void listeners_cleanup(listeners_t *listeners);

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
