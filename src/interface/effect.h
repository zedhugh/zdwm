#pragma once

#include <stddef.h>

#include "base/color.h"
#include "interface/types.h"

typedef enum effect_type_t {
  ZDWM_EFFECT_MAP_WINDOW = 1,
  ZDWM_EFFECT_UNMAP_WINDOW,
  ZDWM_EFFECT_FOCUS_WINDOW,
  ZDWM_EFFECT_KILL_WINDOW,
  ZDWM_EFFECT_WITHDRAW_WINDOW,
  ZDWM_EFFECT_START_MOVE_WINDOW,
  ZDWM_EFFECT_START_RESIZE_WINDOW,
  ZDWM_EFFECT_MINIMIZE_WINDOW,
  ZDWM_EFFECT_MAXIMIZE_WINDOW,
  ZDWM_EFFECT_FULLSCREEN_WINDOW,
  ZDWM_EFFECT_CONFIGURE_WINDOW,
  ZDWM_EFFECT_CONFIGURE_NOTIFY,
  ZDWM_EFFECT_CHANGE_BORDER_COLOR,
  ZDWM_EFFECT_CHANGE_WINDOW_LIST,
  ZDWM_EFFECT_RESTACK_WINDOWS,
  ZDWM_EFFECT_BIND_KEY,
  ZDWM_EFFECT_GRAB_BUTTON,
  ZDWM_EFFECT_UNGRAB_POINTER,
} effect_type_t;

typedef struct effect_move_window_t {
  window_id_t window;
  point_t left_top_point;
} effect_move_window_t;

typedef struct effect_resize_window_t {
  window_id_t window;
  int32_t width;
  int32_t height;
} effect_resize_window_t;

typedef struct effect_change_border_color_t {
  window_id_t window;
  const color_t *color;
} effect_change_border_color_t;

typedef struct effect_window_list_t {
  const window_id_t *windows;
  size_t count;
} effect_window_list_t;

typedef struct key_bind_t {
  modifier_mask_t modifiers;
  keysym_t keysym;
} key_bind_t;

typedef struct effect_bind_key_t {
  const key_bind_t *keys;
  size_t count;
} effect_bind_key_t;

typedef struct grab_button_t {
  modifier_mask_t modifiers;
  button_t button;
} grab_button_t;

typedef struct effect_grab_button_t {
  window_id_t window;
  const grab_button_t *buttons;
  size_t count;
} effect_grab_button_t;

typedef struct effect_bool_window_t {
  window_id_t window;
  bool value;
} effect_bool_window_t;

typedef struct effect_t {
  effect_type_t type;
  union {
    only_window_data_t map;
    only_window_data_t unmap;
    only_window_data_t focus;
    only_window_data_t kill;
    only_window_data_t withdraw;
    only_window_data_t move;
    only_window_data_t resize;
    effect_bool_window_t minimize;
    effect_bool_window_t maximize;
    effect_bool_window_t fullscreen;
    configure_data_t configure;
    only_window_data_t configure_notify;
    effect_change_border_color_t change_border_color;
    effect_window_list_t change_window_list;
    effect_window_list_t restack_windows;
    effect_bind_key_t bind_key;
    effect_grab_button_t grab_button;
  } as;
} effect_t;
