#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t wm_window_id_t;
typedef uint16_t wm_workspace_id_t;
typedef uint16_t wm_output_id_t;
typedef uint32_t wm_layout_id_t;

typedef struct wm_point_t {
  int16_t x;
  int16_t y;
} wm_point_t;

typedef struct wm_rect_t {
  int16_t x;
  int16_t y;
  uint16_t w;
  uint16_t h;
} wm_rect_t;

typedef enum wm_window_geometry_mode_t {
  WM_GEOMETRY_NORMAL,
  WM_GEOMETRY_MAXIMIZED,
  WM_GEOMETRY_FULLSCREEN,
  WM_GEOMETRY_MINIMIZED,
} wm_window_geometry_mode_t;

typedef enum wm_focus_direction_t {
  WM_FOCUS_PREV,
  WM_FOCUS_NEXT,
} wm_focus_direction_t;

typedef enum wm_cross_output_policy_t {
  WM_CROSS_OUTPUT_KEEP_WORKSPACE,
  WM_CROSS_OUTPUT_MOVE_TO_TARGET_WORKSPACE,
} wm_cross_output_policy_t;
