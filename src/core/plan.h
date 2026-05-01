#pragma once

#include <stddef.h>
#include <stdint.h>

#include "base/color.h"
#include "core/types.h"

typedef enum effect_type_t {
  ZDWM_EFFECT_MAP_WINDOW = 1,
  ZDWM_EFFECT_UNMAP_WINDOW,
  ZDWM_EFFECT_FOCUS_WINDOW,
  ZDWM_EFFECT_KILL_WINDOW,
  ZDWM_EFFECT_WITHDRAW_WINDOW,
  ZDWM_EFFECT_MINIMIZE_WINDOW,
  ZDWM_EFFECT_MAXIMIZE_WINDOW,
  ZDWM_EFFECT_FULLSCREEN_WINDOW,
  ZDWM_EFFECT_CONFIGURE_WINDOW,
  ZDWM_EFFECT_CHANGE_BORDER_COLOR,
  ZDWM_EFFECT_CHANGE_WINDOW_LIST,
  ZDWM_EFFECT_RESTACK_WINDOWS,
  ZDWM_EFFECT_BIND_KEY,
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

typedef struct effect_bind_key_t {
  const key_bind_t *keys;
  size_t count;
} effect_bind_key_t;

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
    effect_bool_window_t minimize;
    effect_bool_window_t maximize;
    effect_bool_window_t fullscreen;
    configure_data_t configure;
    effect_change_border_color_t change_border_color;
    effect_window_list_t change_window_list;
    effect_window_list_t restack_windows;
    effect_bind_key_t bind_key;
  } as;
} effect_t;

typedef struct plan_t {
  effect_t *effects;
  size_t count;
  size_t capacity;
  bool need_relayout;
} plan_t;

void plan_reset(plan_t *plan);
void plan_cleanup(plan_t *plan);
void plan_push_effect(plan_t *plan, const effect_t *effect);
