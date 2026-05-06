#pragma once

#include "core/types.h"
#include "core/wm_desc.h"

typedef enum command_type_t {
  ZDWM_COMMAND_MANAGE_WINDOW,
  ZDWM_COMMAND_UNMANAGE_WINDOW,
  ZDWM_COMMAND_FOCUS_WINDOW,
  ZDWM_COMMAND_KILL_WINDOW,
  ZDWM_COMMAND_RAISE_WINDOW,
  ZDWM_COMMAND_WITHDRAW_WINDOW,
  ZDWM_COMMAND_CONFIGURE_WINDOW,
  ZDWM_COMMAND_CHANGE_WINDOW_STATE,
  ZDWM_COMMAND_WINDOW_SET_FLOATING,
  ZDWM_COMMAND_WINDOW_SET_STICKY,
  ZDWM_COMMAND_WINDOW_SET_MINIMIZED,
  ZDWM_COMMAND_WINDOW_SET_MAXIMIZED,
  ZDWM_COMMAND_WINDOW_SET_FULLSCREEN,
  ZDWM_COMMAND_SWITCH_WORKSPACE,
} command_type_t;

typedef struct manage_window_command_t {
  workspace_id_t workspace;
  bool floating;
  window_info_t info;
} manage_window_command_t;

typedef struct switch_workspace_command_t {
  workspace_id_t workspace;
} switch_workspace_command_t;

typedef struct window_state_change_command_t {
  window_id_t window;
  window_state_request_type_t type;
  window_state_request_action_t action;
} window_state_change_command_t;

typedef struct window_bool_state_t {
  window_id_t window;
  bool state;
} window_bool_state_t;

/**
 * @brief 非 owning 的命令值对象
 * @details
 * command_t 自身不拥有任何堆内存；其当前所有 payload 中出现的指针字段均为借用
 * 语义。
 *
 * 因此 command_t 支持按值浅拷贝，且不需要单独的 cleanup 接口。若接收方需要在
 * 源数据生命周期之外继续持有相关内容，必须自行复制。
 */
typedef struct command_t {
  command_type_t type;
  union {
    manage_window_command_t manage_window;
    only_window_data_t unmanage;
    only_window_data_t focus;
    only_window_data_t kill;
    only_window_data_t raise;
    only_window_data_t withdraw;
    configure_data_t configure;
    window_state_change_command_t state_change;
    window_bool_state_t floating;
    window_bool_state_t sticky;
    window_bool_state_t minimized;
    window_bool_state_t maximized;
    window_bool_state_t fullscreen;
    switch_workspace_command_t switch_workspace;
  } as;
} command_t;
