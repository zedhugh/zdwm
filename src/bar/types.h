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
  char *text; /* 持有内存，避免野指针问题 */
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
  int32_t cell_padding;
  int32_t indicator_width;

  void *state;
  zdwm_bar_item_type_t api;
  bool dirty;
  uint64_t last_updated_time;
};

typedef struct bar_side_t {
  zdwm_bar_item_t *items;
  size_t count;
  size_t capacity;
  bar_x_region_t region;
} bar_side_t;
