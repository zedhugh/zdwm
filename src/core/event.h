#pragma once

#include <stdint.h>

#include "core/wm_desc.h"

typedef enum wm_event_type_t {
  WM_EVENT_KEY_PRESS,
  WM_EVENT_POINTER_BUTTON_PRESS,
  WM_EVENT_POINTER_BUTTON_RELEASE,
  WM_EVENT_POINTER_MOTION,
  WM_EVENT_WINDOW_MAP_REQUEST,
  WM_EVENT_WINDOW_REMOVE,
  WM_EVENT_WINDOW_METADATA_CHANGED,
  WM_EVENT_WINDOW_HINTS_CHANGED,
  WM_EVENT_WINDOW_ACTIVATE_REQUEST,
  WM_EVENT_WINDOW_STATE_REQUEST,
  WM_EVENT_CONFIGURE_REQUEST,
} wm_event_type_t;

typedef enum wm_modifier_bit_t {
  WM_MOD_SHIFT = 1u << 0,
  WM_MOD_CTRL = 1u << 1,
  WM_MOD_ALT = 1u << 2,
  WM_MOD_SUPER = 1u << 3,
} wm_modifier_bit_t;

typedef uint32_t wm_modifier_mask_t;
typedef uint32_t wm_keysym_t;

typedef struct wm_key_press_event_t {
  wm_modifier_mask_t modifiers;
  wm_keysym_t keysym;
  uint32_t keycode;
} wm_key_press_event_t;

typedef enum wm_button_t {
  WM_BUTTON_LEFT,
  WM_BUTTON_RIGHT,
  WM_BUTTON_MIDDLE,
} wm_button_t;

typedef struct wm_pointer_button_event_t {
  wm_modifier_mask_t modifiers;
  wm_button_t button;
  wm_window_id_t window;
  wm_point_t root;
  wm_point_t local;
} wm_pointer_button_event_t;

typedef struct wm_pointer_motion_event_t {
  wm_window_id_t window;
  wm_point_t root;
  wm_point_t local;
} wm_pointer_motion_event_t;

typedef struct wm_window_map_request_event_t {
  wm_window_info_t info;
  wm_window_id_t transient_for;
  bool is_dialog;
} wm_window_map_request_event_t;

typedef enum wm_window_remove_reason_t {
  WM_WINDOW_REMOVE_WITHDRAWN,
  WM_WINDOW_REMOVE_DESTROY,
} wm_window_remove_reason_t;

typedef struct wm_window_remove_event_t {
  wm_window_id_t window;
  wm_window_remove_reason_t reason;
} wm_window_remove_event_t;

typedef enum wm_window_metadata_change_flags_t {
  WM_WINDOW_METADATA_CHANGE_TITLE = 1u << 0,
  WM_WINDOW_METADATA_CHANGE_APP_ID = 1u << 1,
  WM_WINDOW_METADATA_CHANGE_CLASS = 1u << 2,
  WM_WINDOW_METADATA_CHANGE_INSTANCE = 1u << 3,
} wm_window_metadata_change_flags_t;

typedef struct wm_window_metadata_change_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  const char *title;
  const char *app_id;
  const char *class_name;
  const char *instance_name;
} wm_window_metadata_change_event_t;

typedef enum wm_window_hint_changed_flags_t {
  WM_WINDOW_HINT_CHANGED_NONE = 0,
  WM_WINDOW_HINT_CHANGED_URGENT = 1u << 0,
  WM_WINDOW_HINT_CHANGED_FIXED_SIZE = 1u << 1,
  WM_WINDOW_HINT_CHANGED_SKIP_TASKBAR = 1u << 2,
} wm_window_hint_changed_flags_t;

typedef struct wm_window_hints_changed_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;
} wm_window_hints_changed_event_t;

typedef enum wm_window_activation_source_t {
  WM_WINDOW_ACTIVATION_SOURCE_LEGACY = 0,
  WM_WINDOW_ACTIVATION_SOURCE_APPLICATION = 1,
  WM_WINDOW_ACTIVATION_SOURCE_PAGER = 2,
} wm_window_activation_source_t;

typedef struct wm_window_activate_request_event_t {
  wm_window_id_t window;
  wm_window_activation_source_t source;
} wm_window_activate_request_event_t;

typedef enum wm_window_state_request_kind_t {
  WM_WINDOW_STATE_REQUEST_FULLSCREEN,
  WM_WINDOW_STATE_REQUEST_MAXIMIZED,
  WM_WINDOW_STATE_REQUEST_MINIMIZED,
} wm_window_state_request_kind_t;

typedef enum wm_window_state_request_action_t {
  WM_WINDOW_STATE_REQUEST_ACTION_ADD,
  WM_WINDOW_STATE_REQUEST_ACTION_REMOVE,
  WM_WINDOW_STATE_REQUEST_ACTION_TOGGLE,
} wm_window_state_request_action_t;

typedef struct wm_window_state_request_event_t {
  wm_window_id_t window;
  wm_window_state_request_kind_t kind;
  wm_window_state_request_action_t action;
} wm_window_state_request_event_t;

typedef enum wm_configure_request_field_t {
  WM_CONFIGURE_REQUEST_FIELD_NONE = 0,
  WM_CONFIGURE_REQUEST_FIELD_X = 1u << 0,
  WM_CONFIGURE_REQUEST_FIELD_Y = 1u << 1,
  WM_CONFIGURE_REQUEST_FIELD_WIDTH = 1u << 2,
  WM_CONFIGURE_REQUEST_FIELD_HEIGHT = 1u << 3,
} wm_configure_request_field_t;

typedef struct wm_configure_request_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} wm_configure_request_event_t;

typedef struct wm_event_t {
  wm_event_type_t type;
  uint32_t time_ms;
  union {
    wm_key_press_event_t key_press;
    wm_pointer_button_event_t pointer_button_press;
    wm_pointer_button_event_t pointer_button_release;
    wm_pointer_motion_event_t pointer_motion;
    wm_window_map_request_event_t window_map_request;
    wm_window_remove_event_t window_remove;
    wm_window_metadata_change_event_t window_metadata_change;
    wm_window_hints_changed_event_t window_hints_changed;
    wm_window_activate_request_event_t window_activate_request;
    wm_window_state_request_event_t window_state_request;
    wm_configure_request_event_t configure_request;
  } data;
} wm_event_t;
