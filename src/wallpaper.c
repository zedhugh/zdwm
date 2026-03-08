#include "wallpaper.h"

#include <Imlib2.h>
#include <cairo-xcb.h>
#include <cairo.h>
#include <glib.h>
#include <glibconfig.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <wordexp.h>
#include <xcb/xproto.h>

#include "atoms-extern.h"
#include "base.h"
#include "types.h"
#include "utils.h"
#include "xwindow.h"

struct wallpaper_context_t {
  uint32_t interval;
  uint32_t image_count;
  char **image_path_list;

  xcb_screen_t *screen;
  xcb_connection_t *conn;

  area_t *monitor_geometries;
  uint32_t monitor_count;

  xcb_pixmap_t pixmap;
  cairo_t *cr;

  uint32_t index;

  guint timer;
};

static inline bool file_exist(const char *path) {
  if (!path || !*path) return false;
  return access(path, R_OK) == 0;
}

static char **parse_wallpaper_paths(char **images, uint32_t image_count) {
  GStrvBuilder *builder = g_strv_builder_new();
  for (uint32_t i = 0; i < image_count; i++) {
    wordexp_t p;
    if (wordexp(images[i], &p, 0)) continue;

    for (size_t i = 0; i < p.we_wordc; i++) {
      size_t len = strlen(p.we_wordv[i]) + 1;
      char *realpath = p_new(char, len);
      strncpy(realpath, p.we_wordv[i], len);
      g_strstrip(realpath);
      if (file_exist(realpath)) g_strv_builder_add(builder, realpath);

      p_delete(&realpath);
    }
    wordfree(&p);
  }

  return g_strv_builder_unref_to_strv(builder);
}

static uint32_t *generate_wallpaper(wallpaper_context_t *context) {
  int width = context->screen->width_in_pixels;
  int height = context->screen->height_in_pixels;

  Imlib_Image final_image = imlib_create_image(width, height);
  if (!final_image) return nullptr;

  imlib_context_set_image(final_image);
  imlib_image_set_has_alpha(true);

  /* 重置像素数据 */
  imlib_image_clear();
  for (uint32_t i = 0; i < context->monitor_count; i++) {
    uint32_t img_index = (context->index + i) % context->image_count;
    const char *path = context->image_path_list[img_index];
    logger("== wallpaper[%u]: %s\n", img_index, path);
    Imlib_Image wallpaper_img = imlib_load_image(path);
    if (!wallpaper_img) continue;

    uint32_t dst_width = context->monitor_geometries[i].width;
    uint32_t dst_height = context->monitor_geometries[i].height;

    /* 获取图片原始尺寸 */
    imlib_context_set_image(wallpaper_img);
    uint32_t img_width = imlib_image_get_width();
    uint32_t img_height = imlib_image_get_height();

    /* 计算缩放比例，保持原始宽高比 */
    float width_ratio = (float)dst_width / img_width;
    float height_ratio = (float)dst_height / img_height;
    float scale = MAX(width_ratio, height_ratio);

    /* 图片居中显示，计算原图绘制区域 */
    int src_width = dst_width / scale;
    int src_height = dst_height / scale;
    int src_x = (img_width - src_width) / 2;
    int src_y = (img_height - src_height) / 2;

    /* 绘制图片到最终图像上 */
    imlib_context_set_image(final_image);
    imlib_blend_image_onto_image(wallpaper_img, true, src_x, src_y, src_width,
                                 src_height, context->monitor_geometries[i].x,
                                 context->monitor_geometries[i].y, dst_width,
                                 dst_height);

    /* 释放图片资源 */
    imlib_context_set_image(wallpaper_img);
    imlib_free_image();
  }

  /* 获取最终图像数据 */
  imlib_context_set_image(final_image);
  uint32_t *data = imlib_image_get_data_for_reading_only();

  /* 复制数据到自己分配的内存中（原数据会在图像释放后无效） */
  int count = width * height;
  uint32_t *result = p_new(uint32_t, count);
  if (result) memcpy(result, data, count * sizeof(uint32_t));

  /* 释放图像 */
  imlib_context_set_image(final_image);
  imlib_free_image();

  return result;
}

double get_time() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void cairo_set_wallpaper(wallpaper_context_t *context,
                                uint8_t *wallpaper_data) {
  xcb_connection_t *conn = context->conn;
  xcb_window_t root = context->screen->root;
  xcb_pixmap_t pixmap = context->pixmap;
  uint32_t width = context->screen->width_in_pixels;
  uint32_t height = context->screen->height_in_pixels;
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  cairo_surface_t *s = cairo_image_surface_create_for_data(
    wallpaper_data, CAIRO_FORMAT_ARGB32, width, height, stride);
  cairo_set_source_surface(context->cr, s, 0, 0);
  cairo_paint(context->cr);

  xcb_change_window_attributes_value_list_t value_list = {
    .background_pixmap = pixmap,
  };
  xcb_change_window_attributes_aux(conn, root, XCB_CW_BACK_PIXMAP, &value_list);

  /* 即使壁纸对应的 pixmap 未变更，也强制通知合成器壁纸已更改 */
  uint8_t mode = XCB_PROP_MODE_REPLACE;
  xcb_pixmap_t none = XCB_PIXMAP_NONE;
  xcb_atom_t atom = _XROOTPMAP_ID;
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &none);
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &pixmap);
  atom = ESETROOT_PMAP_ID;
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &none);
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &pixmap);

  xcb_clear_area(conn, 0, root, 0, 0, 0, 0);
  xcb_flush(conn);
  cairo_surface_destroy(s);
}

static void set_wallpaper(wallpaper_context_t *context) {
  double start = get_time();
  uint32_t *wallpaper = generate_wallpaper(context);
  logger("== generate wallpaper cost: %lf, %p\n", get_time() - start,
         wallpaper);
  if (!wallpaper) return;

  uint8_t *data = (uint8_t *)wallpaper;

  cairo_set_wallpaper(context, data);

  p_delete(&wallpaper);

  context->index += context->monitor_count;
}

static gboolean update_wallpaper(gpointer user_data) {
  if (!user_data) return FALSE;

  wallpaper_context_t *wc = user_data;
  set_wallpaper(wc);

  /* 屏幕比壁纸多则不动态切换 */
  if (wc->monitor_count >= wc->image_count) return FALSE;

  return TRUE;
}

static bool prepare_wallpaper_surface(wallpaper_context_t *context) {
  uint16_t width = context->screen->width_in_pixels;
  uint16_t height = context->screen->height_in_pixels;
  xcb_window_t root = context->screen->root;
  xcb_connection_t *conn = context->conn;
  xcb_pixmap_t pixmap = context->pixmap;

  visual_t *visual = xwindow_get_xcb_visual(false);

  if (pixmap != XCB_PIXMAP_NONE) {
    xcb_kill_client(conn, pixmap);
    xcb_free_pixmap(conn, pixmap);
  }

  pixmap = xcb_generate_id(conn);
  xcb_create_pixmap(conn, visual->depth, pixmap, root, width, height);
  xcb_gcontext_t gc = xcb_generate_id(conn);
  xcb_create_gc(conn, gc, pixmap, 0, nullptr);
  xcb_change_window_attributes_value_list_t value_list = {
    .background_pixmap = pixmap,
  };
  xcb_change_window_attributes_aux(conn, root, XCB_CW_BACK_PIXMAP, &value_list);

  /* 兼容合成器，不设置若开启合成器壁纸会不显示 */
  uint8_t mode = XCB_PROP_MODE_REPLACE;
  xcb_atom_t atom = _XROOTPMAP_ID;
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &pixmap);
  atom = ESETROOT_PMAP_ID;
  xcb_change_property(conn, mode, root, atom, XCB_ATOM_PIXMAP, 32, 1, &pixmap);

  xcb_clear_area(conn, 0, root, 0, 0, width, height);
  xcb_free_gc(conn, gc);
  xcb_flush(conn);

  context->pixmap = pixmap;

  if (context->cr) cairo_destroy(context->cr);
  cairo_surface_t *surface =
    cairo_xcb_surface_create(conn, pixmap, visual->visual, width, height);
  p_delete(&visual);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    return false;
  }

  cairo_t *cr = cairo_create(surface);
  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return false;
  }

  context->cr = cr;

  cairo_surface_destroy(surface);
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_paint(cr);
  xcb_clear_area(conn, 0, root, 0, 0, 0, 0);
  xcb_flush(context->conn);

  return true;
}

wallpaper_context_t *wallpaper_init(wallpaper_config_t config) {
  if (!config.path_count || !config.paths) return nullptr;
  if (!config.monitors) return nullptr;

  char **images = parse_wallpaper_paths(config.paths, config.path_count);
  if (!images || !g_strv_length(images)) return nullptr;

  wallpaper_context_t *ctx = p_new(wallpaper_context_t, 1);
  ctx->interval = config.interval;
  ctx->image_path_list = images;
  ctx->image_count = g_strv_length(images);

  ctx->conn = config.conn;
  ctx->screen = config.screen;

  {
    uint32_t i = 0;
    monitor_t *m = nullptr;
    for (m = config.monitors; m; m = m->next) ctx->monitor_count++;

    ctx->monitor_geometries = p_new(area_t, ctx->monitor_count);
    for (m = config.monitors; m; m = m->next) {
      ctx->monitor_geometries[i++] = m->geometry;
    }
  }

  if (!prepare_wallpaper_surface(ctx)) {
    wallpaper_clean(ctx);
    return nullptr;
  }

  if (!update_wallpaper(ctx) || !ctx->interval) {
    wallpaper_clean(ctx);
    return nullptr;
  }

  ctx->timer = g_timeout_add_seconds(ctx->interval, update_wallpaper, ctx);

  return ctx;
}

void wallpaper_clean(wallpaper_context_t *context) {
  if (!context) return;

  if (context->timer) g_source_remove(context->timer);
  if (context->cr) cairo_destroy(context->cr);
  p_delete(&context->monitor_geometries);
  if (context->image_path_list) g_strfreev(context->image_path_list);

  p_delete(&context);
}
