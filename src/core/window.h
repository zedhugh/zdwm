#pragma once

#include <stddef.h>

#include "core/types.h"

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

typedef struct window_props_t {
  window_id_t transient_for;

  bool override_redirect;
  bool skip_taskbar;
  bool urgent;

  window_type_t *types;
  size_t type_count;

  window_state_t *states;
  size_t state_count;
} window_props_t;

typedef struct window_metadata_t {
  char *title;
  char *app_id;
  char *role;
  char *class_name;
  char *instance_name;
} window_metadata_t;

typedef struct window_geometry_t {
  window_geometry_mode_t mode;
  rect_t rect;
  bool fixed_size;
} window_geometry_t;
