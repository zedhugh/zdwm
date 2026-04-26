#pragma once

#include <stdint.h>
#include <zdwm/rules.h>
#include <zdwm/types.h>

/* clang-format off */
typedef zdwm_layout_id_t        layout_id_t;
typedef zdwm_window_id_t        window_id_t;
typedef zdwm_workspace_id_t     workspace_id_t;
typedef uint32_t                output_id_t;

typedef zdwm_rect_t             rect_t;
typedef zdwm_output_info_t      output_info_t;

typedef zdwm_rule_match_t       rule_match_t;
typedef zdwm_rule_action_t      rule_action_t;
/* clang-format on */

typedef uint32_t modifier_mask_t;
typedef uint32_t keysym_t;

#define ZDWM_OUTPUT_ID_INVALID ((output_id_t)UINT32_MAX)

typedef struct point_t {
  int32_t x;
  int32_t y;
} point_t;

typedef struct color_t {
  uint32_t rgba;
  uint32_t argb;

  /* RGBA color channels for Cairo, each channel in the range [0, 1] */
  double red, green, blue, alpha;
} color_t;

typedef enum window_geometry_mode_t {
  ZDWM_GEOMETRY_NORMAL,
  ZDWM_GEOMETRY_MAXIMIZED,
  ZDWM_GEOMETRY_FULLSCREEN,
  ZDWM_GEOMETRY_MINIMIZED,
} window_geometry_mode_t;

typedef enum focus_direction_t {
  ZDWM_FOCUS_PREV,
  ZDWM_FOCUS_NEXT,
} focus_direction_t;

typedef enum cross_output_policy_t {
  ZDWM_CROSS_OUTPUT_KEEP_WORKSPACE,
  ZDWM_CROSS_OUTPUT_MOVE_TO_TARGET_WORKSPACE,
} cross_output_policy_t;

typedef enum modifier_bit_t {
  ZDWM_MOD_NONE    = 0u,
  ZDWM_MOD_SHIFT   = 1u << 0,
  ZDWM_MOD_CONTROL = 1u << 1,
  ZDWM_MOD_1       = 1u << 2,
  ZDWM_MOD_2       = 1u << 3,
  ZDWM_MOD_3       = 1u << 4,
  ZDWM_MOD_4       = 1u << 5,
  ZDWM_MOD_5       = 1u << 6,
} modifier_bit_t;
