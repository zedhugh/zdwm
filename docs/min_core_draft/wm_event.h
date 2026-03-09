#pragma once

#include "wm_types.h"

typedef enum wm_event_type_t {
  WM_EVENT_NONE,
  WM_EVENT_KEY_PRESS,
  WM_EVENT_BUTTON_PRESS,
  WM_EVENT_BUTTON_RELEASE,
  WM_EVENT_POINTER_MOTION,
  WM_EVENT_MAP_REQUEST,
  WM_EVENT_UNMAP_NOTIFY,
  WM_EVENT_DESTROY_NOTIFY,
  WM_EVENT_WINDOW_METADATA_CHANGED,
  WM_EVENT_CONFIGURE_REQUEST,
  WM_EVENT_OUTPUT_CHANGED,
  WM_EVENT_TIMER_TICK,
  WM_EVENT_STATUS_TICK,
  WM_EVENT_QUIT,
} wm_event_type_t;

typedef struct wm_key_press_event_t {
  uint32_t keycode;
  uint32_t modifiers;
} wm_key_press_event_t;

typedef struct wm_button_event_t {
  uint32_t button;
  uint32_t modifiers;
  wm_point_t root;
  wm_window_id_t target_window_id;
} wm_button_event_t;

typedef struct wm_pointer_motion_event_t {
  wm_point_t root;
  wm_window_id_t target_window_id;
} wm_pointer_motion_event_t;

typedef struct wm_map_request_event_t {
  wm_window_id_t window_id;
} wm_map_request_event_t;

typedef struct wm_unmap_notify_event_t {
  wm_window_id_t window_id;
} wm_unmap_notify_event_t;

typedef struct wm_destroy_notify_event_t {
  wm_window_id_t window_id;
} wm_destroy_notify_event_t;

typedef enum wm_window_meta_changed_flags_t {
  WM_WINDOW_META_CHANGED_NONE = 0,
  WM_WINDOW_META_CHANGED_TITLE = 1u << 0,
  WM_WINDOW_META_CHANGED_APP_ID = 1u << 1,
  WM_WINDOW_META_CHANGED_CLASS = 1u << 2,
  WM_WINDOW_META_CHANGED_INSTANCE = 1u << 3,
} wm_window_meta_changed_flags_t;

typedef struct wm_window_metadata_changed_event_t {
  wm_window_id_t window_id;
  uint32_t changed_fields;
} wm_window_metadata_changed_event_t;

typedef struct wm_configure_request_event_t {
  wm_window_id_t window_id;
  wm_rect_t requested_rect;
} wm_configure_request_event_t;

typedef struct wm_output_changed_event_t {
  wm_output_id_t output_id;
  wm_rect_t geometry;
  wm_rect_t workarea;
} wm_output_changed_event_t;

typedef struct wm_timer_tick_event_t {
  uint64_t now_ms;
} wm_timer_tick_event_t;

typedef struct wm_event_t {
  wm_event_type_t type;
  uint64_t ts_ms;
  union {
    wm_key_press_event_t key_press;
    wm_button_event_t button_press;
    wm_button_event_t button_release;
    wm_pointer_motion_event_t pointer_motion;
    wm_map_request_event_t map_request;
    wm_unmap_notify_event_t unmap_notify;
    wm_destroy_notify_event_t destroy_notify;
    wm_window_metadata_changed_event_t window_metadata_changed;
    wm_configure_request_event_t configure_request;
    wm_output_changed_event_t output_changed;
    wm_timer_tick_event_t timer_tick;
  } as;
} wm_event_t;
