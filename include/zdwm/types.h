#ifndef ZDWM_TYPES_H
#define ZDWM_TYPES_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint32_t zdwm_output_id_t;
typedef uint32_t zdwm_layout_id_t;
typedef uint32_t zdwm_window_id_t;
typedef uint32_t zdwm_workspace_id_t;
typedef uint32_t zdwm_binding_mode_id_t;

#define ZDWM_LAYOUT_ID_INVALID       ((zdwm_layout_id_t)UINT32_MAX)
#define ZDWM_WINDOW_ID_INVALID       ((zdwm_window_id_t)0)
#define ZDWM_WORKSPACE_ID_INVALID    ((zdwm_workspace_id_t)UINT32_MAX)
#define ZDWM_BINDING_MODE_ID_INVALID ((zdwm_binding_mode_id_t)UINT32_MAX)

typedef enum zdwm_button_t {
  ZDWM_BUTTON_NONE,
  ZDWM_BUTTON_LEFT,
  ZDWM_BUTTON_RIGHT,
  ZDWM_BUTTON_MIDDLE,
} zdwm_button_t;

/* clang-format off */
typedef enum zdwm_modifier_bit_t {
  ZDWM_MOD_NONE    = 0u,
  ZDWM_MOD_SHIFT   = 1u << 0,
  ZDWM_MOD_CONTROL = 1u << 1,
  ZDWM_MOD_1       = 1u << 2,
  ZDWM_MOD_2       = 1u << 3,
  ZDWM_MOD_3       = 1u << 4,
  ZDWM_MOD_4       = 1u << 5,
  ZDWM_MOD_5       = 1u << 6,
} zdwm_modifier_bit_t;
/* clang-format on */

typedef uint32_t zdwm_modifier_mask_t;

typedef struct zdwm_rect_t {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} zdwm_rect_t;

typedef struct zdwm_output_info_t {
  const char *name;
  zdwm_rect_t geometry;
} zdwm_output_info_t;

typedef enum zdwm_icon_type_t {
  ZDWM_ICON_TEXT,
  ZDWM_ICON_IMAGE,
} zdwm_icon_type_t;

typedef struct zdwm_icon_t {
  zdwm_icon_type_t type;
  union {
    const char *text;
    const char *image_path;
  } as;
} zdwm_icon_t;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_TYPES_H */
