#ifndef ZDWM_BAR_H
#define ZDWM_BAR_H

#include <stddef.h>
#include <stdint.h>
#include <zdwm/action.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct zdwm_bar_config_t {
  int32_t height;
  bool show_top; /* 如果为 false 则 bar 显示在屏幕底部 */
  uint8_t padding_x;
  uint16_t fps;
  const char *font_family;
  uint32_t font_size;
  uint32_t dpi;

  const char *bg;
  const char *fg;
  const char *layout_bg;
  const char *layout_fg;
  const char *binding_mode_bg;
  const char *binding_mode_fg;
  const char *window_bg;
  const char *window_fg;
  const char *window_focused_bg;
  const char *window_focused_fg;
} zdwm_bar_config_t;

typedef struct zdwm_bar_item_t zdwm_bar_item_t;

typedef struct zdwm_bar_cell_api_t {
  size_t (*get_cell_count)(zdwm_bar_item_t *item);
  void (*set_cell_count)(zdwm_bar_item_t *item, size_t count);

  void (*cell_set_text)(zdwm_bar_item_t *item, size_t index, const char *text);
  void (*cell_set_bg)(zdwm_bar_item_t *item, size_t index, const char *color);
  void (*cell_set_fg)(zdwm_bar_item_t *item, size_t index, const char *color);
  void (*cell_set_icon)(zdwm_bar_item_t *item, size_t index, zdwm_icon_t icon);
  void (*cell_set_indicator)(zdwm_bar_item_t *item, size_t index, bool show);
} zdwm_bar_cell_api_t;

typedef struct zdwm_bar_item_type_t {
  void *(*create_state)(zdwm_output_id_t output_id, void *config);
  void (*update)(
    zdwm_bar_item_t *item,
    const zdwm_bar_cell_api_t *cells,
    void *state
  );
  zdwm_action_t (*on_click)(
    zdwm_bar_item_t *item,
    size_t cell_index,
    int32_t x,
    void *state
  );
  void (*destroy_state)(void *state);
  uint32_t update_interval_ms;
} zdwm_bar_item_type_t;

typedef enum zdwm_bar_side_type_t {
  ZDWM_BAR_SIDE_LEFT,
  ZDWM_BAR_SIDE_RIGHT,
  ZDWM_BAR_SIDE_CENTER_REST, /* 中央剩余部分，用于显示窗口列表 */
  ZDWM_BAR_SIDE_COUNT,
} zdwm_bar_side_type_t;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_BAR_H */
