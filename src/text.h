#pragma once

#include <cairo.h>
#include <stdint.h>

#include "base.h"
#include "color.h"

int text_init_pango_layout(const char *family, uint32_t size, uint32_t dpi);
void text_clean_pango_layout(void);
void text_get_size(const char *text, int *width, int *height);
void draw_text(cairo_t *cr, const char *text, color_t *color, area_t area,
               bool align_center);
void draw_rect(cairo_t *cr, area_t area, bool fill, color_t *color,
               uint16_t line_width);
void draw_background(cairo_t *cr, color_t *color, area_t area);
