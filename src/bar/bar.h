#pragma once

#include <cairo.h>
#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/types.h"

typedef struct bar_output_t {
  cairo_t *cr;
  zdwm_window_id_t window_id;
  zdwm_output_id_t output_id;
  bar_side_t sides[ZDWM_BAR_SIDE_COUNT];
} bar_output_t;

typedef struct bar_t {
  bar_output_t *bars;
  size_t count;

  bool visible;

  zdwm_bar_config_t config;
} bar_t;
