#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ID 类型定义
// workspace/output/layout 在启动时分配固定数量，运行时不再增删
// window 由后端管理，使用后端原生 ID
typedef uint32_t wm_window_id_t;
typedef uint16_t wm_workspace_id_t;
typedef uint16_t wm_output_id_t;
typedef uint32_t wm_layout_id_t;

// 无效 ID 标记
#define WM_WINDOW_ID_INVALID     ((wm_window_id_t)0)
#define WM_WORKSPACE_ID_INVALID  ((wm_workspace_id_t)-1)
#define WM_OUTPUT_ID_INVALID     ((wm_output_id_t)-1)
#define WM_LAYOUT_ID_INVALID     ((wm_layout_id_t)-1)

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
