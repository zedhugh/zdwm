#include "text.h"

#include <glib-object.h>
#include <glibconfig.h>
#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>
#include <stdint.h>
#include <stdio.h>

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
  static const char *layout_family = nullptr;
  static uint32_t layout_size = 0;
  static uint32_t layout_dpi = 0;
  static int text_height = 0;

  if (layout_family != nullptr && strcmp(layout_family, family) == 0 &&
      layout_size == size && layout_dpi == dpi) {
    printf("text pango layout config cached\n");
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
