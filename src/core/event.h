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

typedef struct window_metadata_change_event_t {
  window_id_t window;
  uint32_t changed_fields;
  window_metadata_t metadata;
} window_metadata_change_event_t;

typedef enum window_activation_source_t {
  ZDWM_WINDOW_ACTIVATION_SOURCE_LEGACY      = 0,
  ZDWM_WINDOW_ACTIVATION_SOURCE_APPLICATION = 1,
  ZDWM_WINDOW_ACTIVATION_SOURCE_PAGER       = 2,
} window_activation_source_t;

typedef struct window_activate_request_event_t {
  window_id_t window;
  window_activation_source_t source;
} window_activate_request_event_t;

typedef struct window_state_request_event_t {
  window_id_t window;
  window_state_request_type_t type;
  window_state_request_action_t action;
} window_state_request_event_t;

typedef struct event_t {
  event_type_t type;
  union {
    key_press_event_t key_press;
    pointer_button_event_t pointer_button_press;
    pointer_button_event_t pointer_button_release;
    pointer_motion_event_t pointer_motion;
    only_window_data_t pointer_enter;
    window_map_request_event_t window_map_request;
    window_remove_event_t window_remove;
    window_metadata_change_event_t window_metadata_change;
    window_activate_request_event_t window_activate_request;
    window_state_request_event_t window_state_request;
    configure_data_t configure_request;
  } as;
} event_t;

void event_cleanup(event_t *event);
void event_reset(event_t *event);
