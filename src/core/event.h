#pragma once

#include <stdint.h>

#include "core/types.h"
#include "core/window.h"

typedef enum event_type_t {
  ZDWM_EVENT_KEY_PRESS,
  ZDWM_EVENT_POINTER_BUTTON_PRESS,
  ZDWM_EVENT_POINTER_BUTTON_RELEASE,
  ZDWM_EVENT_POINTER_MOTION,
  ZDWM_EVENT_POINTER_ENTER,
  ZDWM_EVENT_WINDOW_MAP_REQUEST,
  ZDWM_EVENT_WINDOW_REMOVE,
  ZDWM_EVENT_WINDOW_METADATA_CHANGED,
  ZDWM_EVENT_WINDOW_HINTS_CHANGED,
  ZDWM_EVENT_WINDOW_ACTIVATE_REQUEST,
  ZDWM_EVENT_WINDOW_STATE_REQUEST,
  ZDWM_EVENT_CONFIGURE_REQUEST,
} event_type_t;

typedef enum modifier_bit_t {
  ZDWM_MOD_SHIFT = 1u << 0,
  ZDWM_MOD_CTRL = 1u << 1,
  ZDWM_MOD_ALT = 1u << 2,
  ZDWM_MOD_SUPER = 1u << 3,
} modifier_bit_t;

typedef uint32_t modifier_mask_t;
typedef uint32_t keysym_t;

typedef struct key_press_event_t {
  modifier_mask_t modifiers;
  keysym_t keysym;
  uint32_t keycode;
} key_press_event_t;

typedef enum button_t {
  ZDWM_BUTTON_LEFT,
  ZDWM_BUTTON_RIGHT,
  ZDWM_BUTTON_MIDDLE,
} button_t;

typedef struct pointer_button_event_t {
  modifier_mask_t modifiers;
  button_t button;
  window_id_t window;
  point_t root;
  point_t local;
} pointer_button_event_t;

typedef struct pointer_motion_event_t {
  window_id_t window;
  point_t root;
  point_t local;
} pointer_motion_event_t;

typedef struct pointer_enter_event_t {
  window_id_t window;
  point_t root;
  point_t local;
} pointer_enter_event_t;

typedef struct window_map_request_event_t {
  window_id_t window;
  window_id_t transient_for;
  bool override_redirect;
  bool skip_taskbar;
  bool urgent;
  bool fixed_size;
  window_geometry_mode_t geometry_mode;
  rect_t rect;

  window_layer_props_t props;
  window_metadata_t metadata;
} window_map_request_event_t;

typedef enum window_remove_reason_t {
  ZDWM_WINDOW_REMOVE_WITHDRAWN,
  ZDWM_WINDOW_REMOVE_DESTROY,
} window_remove_reason_t;

typedef struct window_remove_event_t {
  window_id_t window;
  window_remove_reason_t reason;
} window_remove_event_t;

typedef enum window_metadata_change_flags_t {
  ZDWM_WINDOW_METADATA_CHANGE_TITLE = 1u << 0,
  ZDWM_WINDOW_METADATA_CHANGE_APP_ID = 1u << 1,
  ZDWM_WINDOW_METADATA_CHANGE_ROLE = 1u << 2,
  ZDWM_WINDOW_METADATA_CHANGE_CLASS = 1u << 3,
  ZDWM_WINDOW_METADATA_CHANGE_INSTANCE = 1u << 4,
} window_metadata_change_flags_t;

typedef struct window_metadata_change_event_t {
  window_id_t window;
  uint32_t changed_fields;
  window_metadata_t metadata;
} window_metadata_change_event_t;

typedef enum window_hint_changed_flags_t {
  ZDWM_WINDOW_HINT_CHANGED_NONE = 0,
  ZDWM_WINDOW_HINT_CHANGED_URGENT = 1u << 0,
  ZDWM_WINDOW_HINT_CHANGED_FIXED_SIZE = 1u << 1,
  ZDWM_WINDOW_HINT_CHANGED_SKIP_TASKBAR = 1u << 2,
} window_hint_changed_flags_t;

typedef struct window_hints_changed_event_t {
  window_id_t window;
  uint32_t changed_fields;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;
} window_hints_changed_event_t;

typedef enum window_activation_source_t {
  ZDWM_WINDOW_ACTIVATION_SOURCE_LEGACY = 0,
  ZDWM_WINDOW_ACTIVATION_SOURCE_APPLICATION = 1,
  ZDWM_WINDOW_ACTIVATION_SOURCE_PAGER = 2,
} window_activation_source_t;

typedef struct window_activate_request_event_t {
  window_id_t window;
  window_activation_source_t source;
} window_activate_request_event_t;

typedef enum window_state_request_kind_t {
  ZDWM_WINDOW_STATE_REQUEST_FULLSCREEN,
  ZDWM_WINDOW_STATE_REQUEST_MAXIMIZED,
  ZDWM_WINDOW_STATE_REQUEST_MINIMIZED,
} window_state_request_kind_t;

typedef enum window_state_request_action_t {
  ZDWM_WINDOW_STATE_REQUEST_ACTION_ADD,
  ZDWM_WINDOW_STATE_REQUEST_ACTION_REMOVE,
  ZDWM_WINDOW_STATE_REQUEST_ACTION_TOGGLE,
} window_state_request_action_t;

typedef struct window_state_request_event_t {
  window_id_t window;
  window_state_request_kind_t kind;
  window_state_request_action_t action;
} window_state_request_event_t;

typedef enum configure_request_field_t {
  ZDWM_CONFIGURE_REQUEST_FIELD_NONE = 0,
  ZDWM_CONFIGURE_REQUEST_FIELD_X = 1u << 0,
  ZDWM_CONFIGURE_REQUEST_FIELD_Y = 1u << 1,
  ZDWM_CONFIGURE_REQUEST_FIELD_WIDTH = 1u << 2,
  ZDWM_CONFIGURE_REQUEST_FIELD_HEIGHT = 1u << 3,
} configure_request_field_t;

typedef struct configure_request_event_t {
  window_id_t window;
  uint32_t changed_fields;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} configure_request_event_t;

typedef struct event_t {
  event_type_t type;
  union {
    key_press_event_t key_press;
    pointer_button_event_t pointer_button_press;
    pointer_button_event_t pointer_button_release;
    pointer_motion_event_t pointer_motion;
    pointer_enter_event_t pointer_enter;
    window_map_request_event_t window_map_request;
    window_remove_event_t window_remove;
    window_metadata_change_event_t window_metadata_change;
    window_hints_changed_event_t window_hints_changed;
    window_activate_request_event_t window_activate_request;
    window_state_request_event_t window_state_request;
    configure_request_event_t configure_request;
  } data;
} event_t;
