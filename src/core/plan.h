#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/types.h"

typedef uint32_t dirty_mask_t;

typedef enum dirty_flags_t : dirty_mask_t {
  ZDWM_DIRTY_NONE   = 0,
  ZDWM_DIRTY_LAYOUT = 1u << 0,
} dirty_flags_t;

typedef enum effect_type_t {
  ZDWM_EFFECT_NONE,
  ZDWM_EFFECT_MAP_WINDOW,
  ZDWM_EFFECT_UNMAP_WINDOW,
  ZDWM_EFFECT_FOCUS_WINDOW,
  ZDWM_EFFECT_MOVE_WINDOW,
  ZDWM_EFFECT_RESIZE_WINDOW,
  ZDWM_EFFECT_CHANGE_BORDER_COLOR,
  ZDWM_EFFECT_CHANGE_BORDER_WIDTH,
  ZDWM_EFFECT_CHANGE_WINDOW_LIST,
  ZDWM_EFFECT_RESTACK_WINDOWS,
} effect_type_t;

typedef struct effect_only_window_id_t {
  window_id_t window;
} effect_only_window_id_t;

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

typedef struct effect_change_border_width_t {
  window_id_t window;
  uint32_t border_width;
} effect_change_border_width_t;

typedef struct effect_window_list_t {
  const window_id_t *windows;
  size_t count;
} effect_window_list_t;

typedef struct effect_t {
  effect_type_t type;
  union {
    effect_only_window_id_t map;
    effect_only_window_id_t unmap;
    effect_only_window_id_t focus;
    effect_move_window_t move;
    effect_resize_window_t resize;
    effect_change_border_color_t change_border_color;
    effect_change_border_width_t change_border_width;
    effect_window_list_t change_window_list;
    effect_window_list_t restack_windows;
  } as;
} effect_t;

typedef struct plan_t {
  effect_t *effects;
  size_t count;
  size_t capacity;
  dirty_mask_t dirty_flags;
} plan_t;

void plan_reset(plan_t *plan);
void plan_cleanup(plan_t *plan);
void plan_push_effect(plan_t *plan, const effect_t *effect);
