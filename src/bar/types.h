#pragma once

#include <stddef.h>
#include <stdint.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "base/color.h"

typedef struct bar_x_region_t {
  int32_t start;
  int32_t end;
} bar_x_region_t;

typedef struct bar_cell_t {
  zdwm_icon_t icon;
  const char *text;
  const color_t *fg;
  const color_t *bg;
  bar_x_region_t region;
  bool show_indicator;
  bool dirty;
} bar_cell_t;

struct zdwm_bar_item_t {
  bar_cell_t *cells;
  size_t count;
  bar_x_region_t region;
  void *state;
  uint8_t cell_padding;
  uint8_t indicator_width;
  bool dirty;
};

typedef struct bar_side_t {
  zdwm_bar_side_type_t type;
  zdwm_bar_item_t *items;
  size_t count;
  size_t capacity;
  bar_x_region_t region;
} bar_side_t;

typedef struct bars_t {
  size_t count;
  int32_t height;
  bool show_top; /* 如果为 false 则 bar 显示屏幕底部 */
  bool visible;

  zdwm_bar_config_t config;
} bars_t;
