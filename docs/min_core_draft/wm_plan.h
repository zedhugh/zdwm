#pragma once

#include "wm_types.h"

typedef enum wm_dirty_flags_t {
  WM_DIRTY_NONE = 0,
  WM_DIRTY_STATE = 1u << 0,
  WM_DIRTY_LAYOUT = 1u << 1,
  WM_DIRTY_STACK = 1u << 2,
  WM_DIRTY_OUTPUT = 1u << 3,
  WM_DIRTY_RENDER = 1u << 4,
} wm_dirty_flags_t;

typedef enum wm_effect_type_t {
  WM_EFFECT_NONE,
  WM_EFFECT_MAP_WINDOW,
  WM_EFFECT_UNMAP_WINDOW,
  WM_EFFECT_CONFIGURE_WINDOW,
  WM_EFFECT_FOCUS_WINDOW,
  WM_EFFECT_RESTACK_WINDOWS,
  WM_EFFECT_RENDER_OUTPUT,
} wm_effect_type_t;

typedef struct wm_effect_t {
  wm_effect_type_t type;
  union {
    struct {
      wm_window_id_t window_id;
    } map_window;

    struct {
      wm_window_id_t window_id;
    } unmap_window;

    struct {
      wm_window_id_t window_id;
      wm_rect_t rect;
      uint16_t border_width;
    } configure_window;

    struct {
      wm_window_id_t window_id;
    } focus_window;

    struct {
      const wm_window_id_t *stack_order;
      size_t stack_count;
    } restack_windows;

    struct {
      wm_output_id_t output_id;
    } render_output;
  } as;
} wm_effect_t;

typedef struct wm_plan_t {
  wm_effect_t *effects;
  size_t effect_count;
  size_t effect_capacity;
  uint32_t dirty_flags;
} wm_plan_t;

void wm_plan_init(wm_plan_t *plan);
void wm_plan_reset(wm_plan_t *plan);
void wm_plan_shutdown(wm_plan_t *plan);

bool wm_plan_push_effect(wm_plan_t *plan, wm_effect_t effect);
