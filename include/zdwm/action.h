#ifndef ZDWM_ACTION_H
#define ZDWM_ACTION_H

#include <stdint.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum zdwm_action_type_t {
  ZDWM_ACTION_NONE,
  ZDWM_ACTION_SPAWN,
  ZDWM_ACTION_QUIT,
  ZDWM_ACTION_RAISE_OR_RUN,

  ZDWM_ACTION_OUTPUT_CYCLE,
  ZDWM_ACTION_WORKSPACE_SWITCH,
  ZDWM_ACTION_WORKSPACE_SWITCH_SAME_OUTPUT_BY_INDEX,
  ZDWM_ACTION_LAYOUT_CYCLE,
  ZDWM_ACTION_BINDING_MODE_CYCLE,

  ZDWM_ACTION_WINDOW_TOGGLE_FULLSCREEN,
  ZDWM_ACTION_WINDOW_TOGGLE_MAXIMIZE,
  ZDWM_ACTION_WINDOW_TOGGLE_MINIMIZE,
  ZDWM_ACTION_WINDOW_TOGGLE_FLOATING,
  ZDWM_ACTION_WINDOW_TOGGLE_STICKY,

  ZDWM_ACTION_WINDOW_FOCUS_CYCLE,
  ZDWM_ACTION_WINDOW_KILL,

  ZDWM_ACTION_WINDOW_SEND_TO_WORKSPACE_SAME_OUTPUT_BY_INDEX,
  ZDWM_ACTION_WINDOW_CYCLE_OUTPUT,

  ZDWM_ACTION_BAR_TOGGLE_VISIBILITY,
} zdwm_action_type_t;

typedef struct zdwm_action_data_spawn_t {
  const char *command;
} zdwm_action_data_spawn_t;

typedef struct zdwm_action_data_quit_t {
  bool restart;
} zdwm_action_data_quit_t;

typedef struct zdwm_action_data_raise_or_run_t {
  const char *class_name;
  const char *command;
} zdwm_action_data_raise_or_run_t;

typedef struct zdwm_action_data_delta_only_t {
  int32_t delta;
} zdwm_action_data_delta_only_t;

typedef struct zdwm_action_data_index_only_t {
  uint32_t index;
} zdwm_action_data_index_only_t;

typedef struct zdwm_action_data_switch_workspace_t {
  zdwm_workspace_id_t workspace;
} zdwm_action_data_switch_workspace_t;

typedef struct zdwm_action_data_window_send_to_workspace_t {
  uint32_t index;
  bool switch_workspace;
} zdwm_action_data_window_send_to_workspace_t;

typedef struct zdwm_action_data_window_cycle_output_t {
  int32_t delta;
  bool keep_focus;
} zdwm_action_data_window_cycle_output_t;

typedef struct zdwm_action_t {
  zdwm_action_type_t type;
  union {
    zdwm_action_data_spawn_t spawn;
    zdwm_action_data_quit_t quit;
    zdwm_action_data_raise_or_run_t raise_or_run;
    zdwm_action_data_delta_only_t output_cycle;
    zdwm_action_data_index_only_t workspace_switch_same_output_by_index;
    zdwm_action_data_switch_workspace_t switch_workspace;
    zdwm_action_data_delta_only_t layout_cycle;
    zdwm_action_data_delta_only_t binding_mode_cycle;
    zdwm_action_data_delta_only_t window_focus_cycle;
    zdwm_action_data_window_send_to_workspace_t
      window_send_to_workspace_same_output_by_index;
    zdwm_action_data_window_cycle_output_t window_cycle_output;
  } as;
} zdwm_action_t;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_ACTION_H */
