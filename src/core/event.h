#pragma once

#include <stdint.h>
#include "core/types.h"

typedef enum wm_event_type_t {
  WM_EVENT_KEY_PRESS,
  WM_EVENT_POINTER_BUTTON_PRESS,
  WM_EVENT_POINTER_BUTTON_RELEASE,
  WM_EVENT_POINTER_MOTION,
  WM_EVENT_WINDOW_MAP_REQUEST,
  WM_EVENT_WINDOW_UNMAP,
  WM_EVENT_WINDOW_DESTROY,
  WM_EVENT_WINDOW_METADATA_CHANGED,
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

typedef struct wm_event_t {
  wm_event_type_t type;
  uint32_t time_ms;
  union {
    wm_key_press_event_t key_press;
    wm_pointer_button_event_t pointer_button_press;
    wm_pointer_button_event_t pointer_button_release;
    wm_pointer_motion_event_t pointer_motion;
  } data;
} wm_event_t;
