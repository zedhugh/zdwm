#include "text.h"

#include <cairo.h>
#include <glib-object.h>
#include <glibconfig.h>
#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-enum-types.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>
#include <stdint.h>
#include <stdio.h>

#include "base.h"
#include "color.h"
#include "utils.h"

static PangoContext *context = nullptr;
static PangoLayout *layout = nullptr;
static PangoAttrList *attr_list = nullptr;

/**
 * 初始化绘制文字的 pango 环境
 * @param family 多个字体用逗号分割
 * @param size 字体大小
 * @param dpi
 * @return layout 高度，可用于确定 bar 高度
 */
int text_init_pango_layout(const char *family, uint32_t size, uint32_t dpi) {
  logger("family: %s, size: %u, dpi: %u\n", family, size, dpi);

  static const char *layout_family = nullptr;
  static uint32_t layout_size = 0;
  static uint32_t layout_dpi = 0;
  static int text_height = 0;

  if (layout_family != nullptr && strcmp(layout_family, family) == 0 &&
      layout_size == size && layout_dpi == dpi) {
    logger("text pango layout config cached\n");
    return text_height;
  }

  text_clean_pango_layout();

  layout_family = family;
  layout_size = size;
  layout_dpi = dpi;

  PangoFontMap *fontmap = pango_cairo_font_map_new();
  context = pango_font_map_create_context(fontmap);
  pango_cairo_context_set_resolution(context, (double)dpi);

  layout = pango_layout_new(context);

  PangoFontDescription *desc = pango_font_description_from_string(family);
  pango_font_description_set_size(desc, size * PANGO_SCALE);
  PangoLanguage *lang = pango_context_get_language(context);
  PangoFontMetrics *metrics = pango_context_get_metrics(context, desc, lang);

  PangoAttribute *attr = pango_attr_font_desc_new(desc);
  attr_list = pango_attr_list_new();
  pango_attr_list_insert(attr_list, attr);
  pango_layout_set_attributes(layout, attr_list);

  pango_layout_set_wrap(layout, PANGO_WRAP_NONE);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

  int height = pango_font_metrics_get_height(metrics);
  int ascent = pango_font_metrics_get_ascent(metrics);
  int descent = pango_font_metrics_get_descent(metrics);
  text_height =
    MAX(PANGO_PIXELS_CEIL(height), PANGO_PIXELS_CEIL(ascent + descent));

  pango_font_metrics_unref(metrics);
  pango_font_description_free(desc);
  g_object_unref(fontmap);

  PangoRectangle ink_rect, logical_rect;
  pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);

  text_height = MAX(text_height, MAX(ink_rect.height, logical_rect.height));

  return text_height;
}

void text_clean_pango_layout(void) {
  if (attr_list) {
    pango_attr_list_unref(attr_list);
    attr_list = nullptr;
  }

  if (layout) {
    g_object_unref(layout);
    layout = nullptr;
  }

  if (context) {
    g_object_unref(context);
    context = nullptr;
  }
}

static void get_layout_size(int *width, int *height) {
  PangoRectangle logical_rect;
  pango_layout_get_pixel_extents(layout, nullptr, &logical_rect);
  if (width) *width = logical_rect.width;
  if (height) *height = logical_rect.height;
}

void text_get_size(const char *text, int *width, int *height) {
  pango_layout_set_width(layout, -1);
  pango_layout_set_text(layout, text, -1);
  get_layout_size(width, height);
}

static void text_layout_prepare(bool align_center) {
  if (layout == nullptr) {
    layout = pango_layout_new(context);
    pango_layout_set_attributes(layout, attr_list);
    pango_layout_set_wrap(layout, PANGO_WRAP_NONE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
  }

  PangoAlignment align = align_center ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT;
  pango_layout_set_alignment(layout, align);
}

void draw_text(cairo_t *cr, const char *text, color_t *color, area_t area,
               bool align_center) {
  text_layout_prepare(align_center);
  pango_layout_set_width(layout, (int)area.width * PANGO_SCALE);
  pango_layout_set_text(layout, text, -1);
  cairo_set_source_rgba(cr, (double)color->red, (double)color->green,
                        (double)color->blue, (double)color->alpha);
  int height = 0;
  get_layout_size(nullptr, &height);

  double offset_y = ((double)area.height - (double)height) / 2;
  pango_cairo_update_layout(cr, layout);
  cairo_move_to(cr, (double)area.x, offset_y + (double)area.y);
  pango_cairo_show_layout(cr, layout);
}

void draw_rect(cairo_t *cr, area_t area, bool fill, color_t *color,
               uint16_t line_width) {
  cairo_set_line_width(cr, (double)line_width);
  double x = (double)area.x;
  double y = (double)area.y;
  double width = (double)area.width;
  double height = (double)area.height;
  if (fill) {
    cairo_rectangle(cr, x, y, width, height);
    cairo_set_source_rgba(cr, (double)color->red, (double)color->green,
                          (double)color->blue, (double)color->alpha);
    cairo_fill(cr);
  } else {
    double half_line_width = (double)line_width / 2;
    x += half_line_width;
    y += half_line_width;
    width -= half_line_width;
    height -= half_line_width;

    cairo_rectangle(cr, x, y, width, height);
    cairo_set_source_rgba(cr, (double)color->red, (double)color->green,
                          (double)color->blue, (double)color->alpha);
    cairo_stroke(cr);
  }
}

void draw_background(cairo_t *cr, color_t *color, area_t area) {
  double x = (double)area.x;
  double y = (double)area.y;
  double width = (double)area.width;
  double height = (double)area.height;
  cairo_move_to(cr, x, y);
  cairo_set_source_rgba(cr, (double)color->red, (double)color->green,
                        (double)color->blue, (double)color->alpha);
  cairo_rectangle(cr, x, y, width, height);
  cairo_fill(cr);
}
