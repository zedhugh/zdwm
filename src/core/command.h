#pragma once

#include "core/types.h"
#include "core/wm_desc.h"

typedef enum command_type_t {
  ZDWM_COMMAND_MANAGE_WINDOW,
  ZDWM_COMMAND_UNMANAGE_WINDOW,
  ZDWM_COMMAND_FOCUS_WINDOW,
  ZDWM_COMMAND_KILL_WINDOW,
  ZDWM_COMMAND_SWITCH_WORKSPACE,
} command_type_t;

typedef struct manage_window_command_t {
  workspace_id_t workspace;
  bool floating;
  window_info_t info;
} manage_window_command_t;

typedef struct switch_workspace_command_t {
  output_id_t output;
  workspace_id_t workspace;
} switch_workspace_command_t;

typedef struct only_window_command_t {
  window_id_t window;
} only_window_command_t;

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
    only_window_command_t unmanage;
    only_window_command_t focus;
    only_window_command_t kill;
    switch_workspace_command_t switch_workspace;
  } as;
} command_t;
