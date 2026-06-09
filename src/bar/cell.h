#pragma once

#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/types.h"

void bar_cell_reset_color_cache(void);
void bar_cell_clean_color_cache(void);

void bar_cell_set_region(
  zdwm_bar_item_t *item,
  size_t index,
  bar_x_region_t region
);

extern zdwm_bar_cell_api_t bar_cell_api;
