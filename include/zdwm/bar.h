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

  uint32_t tag_cell_padding_x;
  uint32_t tag_indicator_width;
  const char *tag_bg;
  const char *tag_fg;
  const char *tag_active_bg;
  const char *tag_active_fg;
  const char *tag_urgent_bg;
  const char *tag_urgent_fg;

  const char *layout_bg;
  const char *layout_fg;

  bool binding_show_default;
  uint32_t binding_padding_x;
  const char *binding_mode_bg;
  const char *binding_mode_fg;

  int32_t tray_output_index;

  uint32_t window_padding_x;
  const char *window_bg;
  const char *window_fg;
  const char *window_focused_bg;
  const char *window_focused_fg;
} zdwm_bar_config_t;

typedef struct zdwm_bar_item_t zdwm_bar_item_t;

typedef struct zdwm_bar_x_region_t {
  int32_t start;
  int32_t end;
} zdwm_bar_x_region_t;

typedef struct zdwm_bar_cell_api_t {
  size_t (*get_cell_count)(zdwm_bar_item_t *item);
  void (*set_cell_count)(zdwm_bar_item_t *item, size_t count);

  void (*cell_set_text)(zdwm_bar_item_t *item, size_t index, const char *text);
  void (*cell_set_bg)(zdwm_bar_item_t *item, size_t index, const char *color);
  void (*cell_set_fg)(zdwm_bar_item_t *item, size_t index, const char *color);
  void (*cell_set_icon)(zdwm_bar_item_t *item, size_t index, zdwm_icon_t icon);
  void (*cell_set_indicator)(zdwm_bar_item_t *item, size_t index, bool show);
  void (*cell_set_fixed_width)(
    zdwm_bar_item_t *item,
    size_t index,
    int32_t width
  );

  zdwm_bar_x_region_t (*cell_get_region)(zdwm_bar_item_t *item, size_t index);
} zdwm_bar_cell_api_t;

typedef struct zdwm_bar_click_params_t {
  zdwm_bar_item_t *item;
  size_t cell_index;
  int32_t x;
  zdwm_modifier_mask_t modifiers;
  zdwm_button_t button;
  void *state;
} zdwm_bar_click_params_t;

typedef struct zdwm_bar_item_hook_params_t {
  zdwm_bar_item_t *item;
  const zdwm_bar_cell_api_t *cell_api;
  zdwm_bar_x_region_t region;
  void *state;
} zdwm_bar_item_hook_params_t;

typedef struct zdwm_bar_item_type_t {
  void *(*create_state)(zdwm_output_id_t output_id, void *config);
  void (*update)(
    zdwm_bar_item_t *item,
    const zdwm_bar_cell_api_t *cells,
    void *state
  );
  void (*after_layout)(const zdwm_bar_item_hook_params_t *params);
  void (*after_draw)(const zdwm_bar_item_hook_params_t *params);
  zdwm_action_t (*on_click)(zdwm_bar_click_params_t *params);
  void (*destroy_state)(void *state);
  zdwm_bar_item_t *instance;
  uint32_t update_interval_ms;
} zdwm_bar_item_type_t;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_BAR_H */
