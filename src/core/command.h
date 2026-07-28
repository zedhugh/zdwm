#pragma once

#include "core/window_info.h"
#include "interface/types.h"

typedef enum command_type_t {
  ZDWM_COMMAND_MANAGE_WINDOW,
  ZDWM_COMMAND_UNMANAGE_WINDOW,
  ZDWM_COMMAND_FOCUS_WINDOW,
  ZDWM_COMMAND_KILL_WINDOW,
  ZDWM_COMMAND_RAISE_WINDOW,
  ZDWM_COMMAND_WITHDRAW_WINDOW,
  ZDWM_COMMAND_CONFIGURE_WINDOW,
  ZDWM_COMMAND_NOTIFY_CONFIGURE,
  ZDWM_COMMAND_CHANGE_WINDOW_STATE,
  ZDWM_COMMAND_START_MOVE_WINDOW,
  ZDWM_COMMAND_STOP_MOVE_WINDOW,
  ZDWM_COMMAND_START_RESIZE_WINDOW,
  ZDWM_COMMAND_STOP_RESIZE_WINDOW,
  ZDWM_COMMAND_WINDOW_SEND_TO_WORKSPACE,
  ZDWM_COMMAND_WINDOW_SET_FLOATING,
  ZDWM_COMMAND_WINDOW_SET_STICKY,
  ZDWM_COMMAND_WINDOW_SET_MINIMIZED,
  ZDWM_COMMAND_WINDOW_SET_MAXIMIZED,
  ZDWM_COMMAND_WINDOW_SET_FULLSCREEN,
  ZDWM_COMMAND_CHANGE_HINTS,
  ZDWM_COMMAND_SWITCH_WORKSPACE,
  ZDWM_COMMAND_SET_CURRENT_OUTPUT,
  ZDWM_COMMAND_SET_LAYOUT,
  ZDWM_COMMAND_SET_BINDING_MODE,
  ZDWM_COMMAND_SET_BAR_VISIBILITY,
  ZDWM_COMMAND_QUIT,
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

typedef struct window_start_move_command_t {
  window_id_t window;
  point_t pointer_coordinate;
} start_interaction_command_t;

typedef struct window_send_to_workspace_command_t {
  window_id_t window;
  workspace_id_t workspace;
} window_send_to_workspace_command_t;

typedef struct window_bool_state_t {
  window_id_t window;
  bool state;
} window_bool_state_t;

typedef struct set_current_output_command_t {
  output_id_t output;
} set_current_output_command_t;

typedef struct set_layout_command_t {
  workspace_id_t workspace;
  layout_id_t layout;
} set_layout_command_t;

typedef struct set_binding_mode_command_t {
  zdwm_binding_mode_id_t mode;
} set_binding_mode_command_t;

typedef struct visibility_command_t {
  bool visible;
} visibility_command_t;

typedef struct quit_command_t {
  bool will_restart;
} quit_command_t;

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
    only_window_data_t notify_configure;
    window_state_change_command_t state_change;
    start_interaction_command_t move;
    start_interaction_command_t resize;
    window_send_to_workspace_command_t send_to_workspace;
    window_bool_state_t floating;
    window_bool_state_t sticky;
    window_bool_state_t minimized;
    window_bool_state_t maximized;
    window_bool_state_t fullscreen;
    hints_data_t hints;
    switch_workspace_command_t switch_workspace;
    set_current_output_command_t current_output;
    set_layout_command_t layout;
    set_binding_mode_command_t binding_mode;
    visibility_command_t bar_visibility;
    quit_command_t quit;
  } as;
} command_t;
