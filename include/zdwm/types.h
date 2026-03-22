#ifndef ZDWM_TYPES_H
#define ZDWM_TYPES_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint32_t zdwm_layout_id_t;
typedef uint32_t zdwm_window_id_t;
typedef uint32_t zdwm_workspace_id_t;

/* clang-format off */
#define ZDWM_LAYOUT_ID_INVALID      ((zdwm_layout_id_t)UINT32_MAX)
#define ZDWM_WINDOW_ID_INVALID      ((zdwm_window_id_t)0)
#define ZDWM_WORKSPACE_ID_INVALID   ((zdwm_workspace_id_t)UINT32_MAX)
/* clang-format on */

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

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_TYPES_H */
