#pragma once

#include <cairo.h>
#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/text.h"
#include "bar/types.h"
#include "base/color.h"

typedef struct bar_output_t {
  cairo_t *cr;
  zdwm_window_id_t window_id;
  zdwm_output_id_t output_id;
  bar_side_t sides[ZDWM_BAR_SIDE_COUNT];
} bar_output_t;

typedef struct bar_palette_t {
  color_t bg;
} bar_palette_t;

typedef struct bar_t {
  bar_output_t *bars;
  size_t count;

  bool visible;

  zdwm_bar_config_t config;
  bar_palette_t palette;
  text_context_t *ctx;
} bar_t;

void bar_init(bar_t *bar);
void bar_cleanup(bar_t *bar);
