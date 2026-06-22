#pragma once

#include <cairo.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/text.h"
#include "bar/types.h"
#include "base/color.h"
#include "core/listeners.h"

typedef struct bar_output_t {
  cairo_t *cr;
  zdwm_window_id_t window_id;
  zdwm_output_id_t output_id;
  int32_t width;
  int32_t height;
  bar_side_t left;
  bar_side_t right;
  zdwm_bar_item_t center;
} bar_output_t;

typedef struct bar_palette_t {
  color_t bg;
} bar_palette_t;

typedef struct bar_t {
  bar_output_t *bars;
  size_t count;

  bool visible;
  int timerfd;

  zdwm_bar_config_t config;
  bar_palette_t palette;
  text_context_t *ctx;
} bar_t;

void bar_init(bar_t *bar, listeners_t *listeners);
void bar_cleanup(bar_t *bar);
void bar_update(bar_t *bar);
bool bar_draw(bar_t *bar);
bool bar_click(
  bar_t *bar,
  zdwm_window_id_t window,
  int32_t x,
  zdwm_action_t *action
);
