#pragma once

#include <cairo.h>
#include <stdint.h>
#include <zdwm/types.h>

#include "base/color.h"

typedef struct text_context_t text_context_t;

text_context_t *
text_context_create(const char *family, uint32_t size, uint32_t dpi);
void text_context_destory(text_context_t *context);
void text_context_get_text_size(
  text_context_t *context,
  const char *text,
  int32_t *width,
  int32_t *height
);
void draw_text(
  cairo_t *cr,
  text_context_t *context,
  const char *text,
  color_t *color,
  zdwm_rect_t area
);
void draw_background(cairo_t *cr, color_t *color, zdwm_rect_t area);
