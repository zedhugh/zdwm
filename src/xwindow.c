#include "xwindow.h"

#include <xcb/xcb_aux.h>

#include "utils.h"
#include "wm.h"
#include "xcursor.h"

xcb_window_t xwindow_create_bar_window(area_t area, uint32_t background_argb,
                                       cursor_t cursor) {
  static xcb_colormap_t colormap = XCB_NONE;
  visual_t *visual = xwindow_get_xcb_visual(true);
  xcb_visualid_t visual_id = visual->visual->visual_id;
  xcb_connection_t *conn = wm.xcb_conn;
  if (colormap == XCB_NONE) {
    colormap = xcb_generate_id(conn);
    uint8_t alloc = XCB_COLORMAP_ALLOC_NONE;
    xcb_create_colormap(conn, alloc, colormap, wm.screen->root, visual_id);
  }

  xcb_window_t window = xcb_generate_id(conn);
  uint32_t value_mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_BACK_PIXEL |
                        XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
                        XCB_CW_CURSOR | XCB_CW_COLORMAP;
  const xcb_create_window_value_list_t value_list = {
    .override_redirect = true,
    .background_pixel = background_argb,
    .border_pixel = 0,
    .event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
    .cursor = xcursor_get_xcb_cursor(cursor),
    .colormap = colormap,
  };
  xcb_void_cookie_t cookie;
  cookie = xcb_create_window_aux_checked(
    conn, visual->depth, window, wm.screen->root, area.x, area.y, area.width,
    area.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual_id, value_mask,
    &value_list);
  p_delete(&visual);
  if (xcb_request_check(conn, cookie)) fatal("cannot create bar window");

  cookie = xcb_map_window(conn, window);
  if (xcb_request_check(conn, cookie)) fatal("cannot map bar window:");

  xcb_aux_sync(conn);

  return window;
}

static visual_t *find_alpha_visual() {
  xcb_depth_iterator_t depth_iter;
  depth_iter = xcb_screen_allowed_depths_iterator(wm.screen);
  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    if (depth_iter.data->depth != 32) continue;

    xcb_visualtype_iterator_t visual_iter;
    visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
        visual_t *r = p_new(visual_t, 1);
        r->visual = visual_iter.data;
        r->depth = depth_iter.data->depth;
        return r;
      }
    }
  }

  return NULL;
}

static visual_t *find_root_visual() {
  xcb_depth_iterator_t depth_iter;
  depth_iter = xcb_screen_allowed_depths_iterator(wm.screen);
  xcb_visualid_t vid = wm.screen->root_visual;
  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter;
    visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (visual_iter.data->visual_id == vid) {
        visual_t *r = p_new(visual_t, 1);
        r->visual = visual_iter.data;
        r->depth = depth_iter.data->depth;
        return r;
      }
    }
  }

  return NULL;
}

/**
 * @brief 获取 xcb 的 visual 及其对应的 depth
 * @param prefer_alpha 支持 alpha 的 visual 优先
 * @returns 返回的信息使用后记得使用 free 释放
 */
visual_t *xwindow_get_xcb_visual(bool prefer_alpha) {
  if (!prefer_alpha) return find_root_visual();

  visual_t *visual = find_alpha_visual();
  if (visual == NULL) visual = find_root_visual();

  return visual;
}
