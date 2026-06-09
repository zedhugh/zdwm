#include "bar/text.h"

#include <assert.h>
#include <cairo.h>
#include <glib-object.h>
#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-font.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>
#include <pango/pangocairo.h>
#include <stdint.h>

#include "base/memory.h"

struct text_context_t {
  PangoContext *context;
  PangoLayout *layout;
  PangoAttrList *attr_list;
};

text_context_t *
text_context_create(const char *family, uint32_t size, uint32_t dpi) {
  auto fontmap = pango_cairo_font_map_new();
  auto context = pango_font_map_create_context(fontmap);
  pango_cairo_context_set_resolution(context, (double)dpi);

  auto desc = pango_font_description_from_string(family);
  pango_font_description_set_size(desc, size * PANGO_SCALE);
  auto attr = pango_attr_font_desc_new(desc);

  auto attr_list = pango_attr_list_new();
  pango_attr_list_insert(attr_list, attr);

  auto layout = pango_layout_new(context);
  pango_layout_set_attributes(layout, attr_list);
  pango_layout_set_wrap(layout, PANGO_WRAP_NONE);
  pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

  pango_font_description_free(desc);
  g_object_unref(fontmap);

  auto ctx = p_new(text_context_t, 1);
  *ctx     = (text_context_t){
        .context   = context,
        .layout    = layout,
        .attr_list = attr_list,
  };

  return ctx;
}

void text_context_destory(text_context_t *context) {
  if (!context) return;

  if (context->layout) {
    g_object_unref(context->layout);
    context->layout = nullptr;
  }

  if (context->attr_list) {
    pango_attr_list_unref(context->attr_list);
    context->attr_list = nullptr;
  }

  if (context->context) {
    g_object_unref(context->context);
    context->context = nullptr;
  }

  p_delete(&context);
}

static inline void
get_layout_size(PangoLayout *layout, int32_t *width, int32_t *height) {
  pango_layout_get_pixel_size(layout, width, height);
}

void text_context_get_text_size(
  text_context_t *context,
  const char *text,
  int32_t *width,
  int32_t *height
) {
  assert(context && text);
  if (!width && !height) return;

  auto layout = context->layout;

  pango_layout_set_width(layout, -1);
  pango_layout_set_height(layout, 0);
  pango_layout_set_text(layout, text, -1);

  get_layout_size(layout, width, height);
}

void draw_text(
  cairo_t *cr,
  text_context_t *context,
  const char *text,
  const color_t *color,
  zdwm_rect_t area
) {
  assert(cr);
  assert(context);
  assert(text);
  assert(color);

  auto layout = context->layout;
  pango_layout_set_width(layout, area.width * PANGO_SCALE);
  pango_layout_set_text(layout, text, -1);

  cairo_set_source_rgba(
    cr,
    color->red,
    color->green,
    color->blue,
    color->alpha
  );

  int32_t height = 0;
  get_layout_size(layout, nullptr, &height);

  double offset_y = (area.height - height) / 2.0;
  pango_cairo_update_layout(cr, layout);
  cairo_move_to(cr, area.x, offset_y + area.y);
  pango_cairo_show_layout(cr, layout);
}

void draw_background(cairo_t *cr, const color_t *color, zdwm_rect_t area) {
  cairo_move_to(cr, area.x, area.y);
  cairo_set_source_rgba(
    cr,
    color->red,
    color->green,
    color->blue,
    color->alpha
  );
  cairo_rectangle(cr, area.x, area.y, area.width, area.height);
  cairo_fill(cr);
}
