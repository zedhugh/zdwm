#pragma once

#include <stdint.h>
#include <zdwm/rules.h>
#include <zdwm/types.h>

#include "base/color.h"

/* clang-format off */
typedef zdwm_layout_id_t        layout_id_t;
typedef zdwm_window_id_t        window_id_t;
typedef zdwm_workspace_id_t     workspace_id_t;
typedef zdwm_output_id_t        output_id_t;

typedef zdwm_rect_t             rect_t;
typedef zdwm_output_info_t      output_info_t;

typedef zdwm_rule_match_t       rule_match_t;
typedef zdwm_rule_action_t      rule_action_t;

typedef enum zdwm_button_t          button_t;
typedef enum zdwm_modifier_bit_t    modifier_bit_t;
typedef zdwm_modifier_mask_t        modifier_mask_t;
/* clang-format on */

typedef uint32_t keysym_t;

#define ZDWM_OUTPUT_ID_INVALID ((output_id_t)UINT32_MAX)

typedef struct point_t {
  int32_t x;
  int32_t y;
} point_t;

typedef enum focus_direction_t {
  ZDWM_FOCUS_PREV,
  ZDWM_FOCUS_NEXT,
} focus_direction_t;

typedef enum cross_output_policy_t {
  ZDWM_CROSS_OUTPUT_KEEP_WORKSPACE,
  ZDWM_CROSS_OUTPUT_MOVE_TO_TARGET_WORKSPACE,
} cross_output_policy_t;

typedef struct key_bind_t {
  modifier_mask_t modifiers;
  keysym_t keysym;
} key_bind_t;

typedef struct grab_button_t {
  modifier_mask_t modifiers;
  button_t button;
} grab_button_t;

typedef enum window_state_request_type_t {
  ZDWM_WINDOW_STATE_REQUEST_FULLSCREEN,
  ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED,
  ZDWM_WINDOW_STATE_REQUEST_MINIMIZED,
  ZDWM_WINDOW_STATE_REQUEST_SKIP_TASKBAR,
} window_state_request_type_t;

typedef enum window_state_request_action_t {
  ZDWM_WINDOW_STATE_ACTION_ADD,
  ZDWM_WINDOW_STATE_ACTION_REMOVE,
  ZDWM_WINDOW_STATE_ACTION_TOGGLE,
} window_state_request_action_t;

/* clang-format off */
typedef enum configure_field_t {
  ZDWM_CONFIGURE_FIELD_X            = 1u << 0,
  ZDWM_CONFIGURE_FIELD_Y            = 1u << 1,
  ZDWM_CONFIGURE_FIELD_WIDTH        = 1u << 2,
  ZDWM_CONFIGURE_FIELD_HEIGHT       = 1u << 3,
  ZDWM_CONFIGURE_FIELD_BORDER_WIDTH = 1u << 4,
  ZDWM_CONFIGURE_FIELD_SIBLING      = 1u << 5,
  ZDWM_CONFIGURE_FIELD_STACK_MODE   = 1u << 6,
} configure_field_t;
/* clang-format on */

typedef struct configure_data_t {
  window_id_t window;
  uint32_t changed_fields;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
  uint32_t border_width;
  window_id_t sibling;
  uint32_t stack_mode;
} configure_data_t;

typedef struct border_config_t {
  uint32_t width;
  color_t normal_color;
  color_t focused_color;
} border_config_t;

typedef struct only_window_data_t {
  window_id_t window;
} only_window_data_t;

typedef enum hint_field_t {
  ZDWM_HINT_FIELD_URGENT,
  ZDWM_HINT_FIELD_SIZE,
} hint_field_t;

typedef struct zdwm_size_t {
  int32_t width;
  int32_t height;
} zdwm_size_t;

typedef struct hints_data_t {
  window_id_t window;
  uint32_t changed_fields;
  bool urgent;
  zdwm_size_t min_size;
  zdwm_size_t max_size;
} hints_data_t;

typedef enum window_interaction_mode_t {
  ZDWM_WINDOW_INTERACTION_NONE,
  ZDWM_WINDOW_INTERACTION_MOVE,
  ZDWM_WINDOW_INTERACTION_RESIZE,
} window_interaction_mode_t;

typedef struct window_interaction_state_t {
  window_interaction_mode_t mode;
  window_id_t window;
  point_t start_coordinate;
  rect_t origin_rect;
  uint64_t last_change_time;
} window_interaction_state_t;

static inline bool window_id_invalid(window_id_t window_id) {
  return window_id == ZDWM_WINDOW_ID_INVALID;
}

static inline bool workspace_id_invalid(workspace_id_t workspace_id) {
  return workspace_id == ZDWM_WORKSPACE_ID_INVALID;
}
