#pragma once

#include <stddef.h>
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

typedef struct zdwm_size_t {
  int32_t width;
  int32_t height;
} zdwm_size_t;

typedef struct only_window_data_t {
  window_id_t window;
} only_window_data_t;

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

/* Internal semantic window types mapped from backend-specific protocols. */
typedef enum window_type_t {
  ZDWM_WINDOW_TYPE_NORMAL = 0,
  ZDWM_WINDOW_TYPE_DESKTOP,
  ZDWM_WINDOW_TYPE_DOCK,
  ZDWM_WINDOW_TYPE_TOOLBAR,
  ZDWM_WINDOW_TYPE_DIALOG,
  ZDWM_WINDOW_TYPE_UTILITY,
  ZDWM_WINDOW_TYPE_SPLASH,
  ZDWM_WINDOW_TYPE_MENU,
  ZDWM_WINDOW_TYPE_DROPDOWN_MENU,
  ZDWM_WINDOW_TYPE_POPUP_MENU,
  ZDWM_WINDOW_TYPE_TOOLTIP,
  ZDWM_WINDOW_TYPE_COMBO,
  ZDWM_WINDOW_TYPE_DND,
  ZDWM_WINDOW_TYPE_NOTIFICATION,
} window_type_t;

/* Internal semantic window states mapped from backend-specific protocols. */
typedef enum window_state_t {
  ZDWM_WINDOW_STATE_ABOVE = 0,
  ZDWM_WINDOW_STATE_FULLSCREEN,
  ZDWM_WINDOW_STATE_MODAL,
  ZDWM_WINDOW_STATE_STICKY,
} window_state_t;

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

/* 层级从低到高 */
typedef enum window_layer_type_t {
  ZDWM_WINDOW_LAYER_DESKTOP = 0,
  ZDWM_WINDOW_LAYER_NORMAL,
  ZDWM_WINDOW_LAYER_TOP,
  ZDWM_WINDOW_LAYER_OVERLAY,
  ZDWM_WINDOW_LAYER_COUNT,
} window_layer_type_t;

typedef struct window_layer_props_t {
  window_type_t *types;
  size_t type_count;

  window_state_t *states;
  size_t state_count;
} window_layer_props_t;

/* clang-format off */
typedef enum window_metadata_change_flags_t {
  ZDWM_WINDOW_METADATA_CHANGE_TITLE    = 1u << 0,
  ZDWM_WINDOW_METADATA_CHANGE_APP_ID   = 1u << 1,
  ZDWM_WINDOW_METADATA_CHANGE_ROLE     = 1u << 2,
  ZDWM_WINDOW_METADATA_CHANGE_CLASS    = 1u << 3,
  ZDWM_WINDOW_METADATA_CHANGE_INSTANCE = 1u << 4,
} window_metadata_change_flags_t;
/* clang-format on */

typedef struct window_metadata_t {
  char *title;
  char *app_id;
  char *role;
  char *class_name;
  char *instance_name;
} window_metadata_t;

typedef enum hint_field_t {
  ZDWM_HINT_FIELD_URGENT,
  ZDWM_HINT_FIELD_SIZE,
} hint_field_t;

typedef struct hints_data_t {
  window_id_t window;
  uint32_t changed_fields;
  bool urgent;
  zdwm_size_t min_size;
  zdwm_size_t max_size;
} hints_data_t;

typedef struct border_config_t {
  uint32_t width;
  color_t normal_color;
  color_t focused_color;
} border_config_t;

static inline bool window_id_invalid(window_id_t window_id) {
  return window_id == ZDWM_WINDOW_ID_INVALID;
}

static inline bool workspace_id_invalid(workspace_id_t workspace_id) {
  return workspace_id == ZDWM_WORKSPACE_ID_INVALID;
}
