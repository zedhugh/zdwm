#include "xwindow.h"

#include <stdint.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "action.h"
#include "atoms-extern.h"
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

  return nullptr;
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

  return nullptr;
}

/**
 * @brief 获取 xcb 的 visual 及其对应的 depth
 * @param prefer_alpha 支持 alpha 的 visual 优先
 * @returns 返回的信息使用后记得使用 free 释放
 */
visual_t *xwindow_get_xcb_visual(bool prefer_alpha) {
  if (!prefer_alpha) return find_root_visual();

  visual_t *visual = find_alpha_visual();
  if (visual == nullptr) visual = find_root_visual();

  return visual;
}

void xwindow_change_cursor(xcb_window_t window, cursor_t cursor) {
  xcb_params_cw_t params = {.cursor = xcursor_get_xcb_cursor(cursor)};
  xcb_aux_change_window_attributes(wm.xcb_conn, window, XCB_CW_CURSOR, &params);
}

void xwindow_grab_keys(xcb_window_t window, const keyboard_t *keys,
                       int keys_length) {
  xcb_connection_t *conn = wm.xcb_conn;
  xcb_ungrab_key(conn, XCB_GRAB_ANY, window, modifier_any);

  xcb_grab_mode_t mode = XCB_GRAB_MODE_ASYNC;
  for (int i = 0; i < keys_length; i++) {
    xcb_keycode_t *keycodes =
      xcb_key_symbols_get_keycode(wm.key_symbols, keys[i].keysym);
    if (keycodes) {
      for (xcb_keycode_t *kc = keycodes; *kc != XCB_NO_SYMBOL; kc++) {
        xcb_grab_key(conn, true, window, keys[i].modifiers, *kc, mode, mode);
      }
      p_delete(&keycodes);
    }
  }
}

bool xwindow_send_event(xcb_window_t window, xcb_atom_t atom) {
  bool exist = false;

  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_protocols(wm.xcb_conn, window, atom);
  xcb_icccm_get_wm_protocols_reply_t reply;
  if (xcb_icccm_get_wm_protocols_reply(wm.xcb_conn, cookie, &reply, nullptr)) {
    for (uint32_t i = 0; !exist && i < reply.atoms_len; i++) {
      exist = reply.atoms[i] == atom;
    }
    xcb_icccm_get_wm_protocols_reply_wipe(&reply);
  }

  if (exist) {
    xcb_client_message_event_t ev;
    p_clear(&ev, 1);
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.window = window;
    ev.type = WM_PROTOCOLS;
    ev.data.data32[0] = atom;
    ev.data.data32[1] = XCB_CURRENT_TIME;
  }

  return exist;
}

void xwindow_focus(xcb_window_t window) {
  if (window == XCB_WINDOW_NONE || window == wm.screen->root) {
    xcb_set_input_focus(wm.xcb_conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                        wm.screen->root, XCB_CURRENT_TIME);
    xcb_delete_property(wm.xcb_conn, wm.screen->root, _NET_ACTIVE_WINDOW);
  } else {
    xcb_set_input_focus(wm.xcb_conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
                        XCB_CURRENT_TIME);
    xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, wm.screen->root,
                        _NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &window);
    xwindow_send_event(window, WM_TAKE_FOCUS);
  }
}

/**
 * 获取目标窗口的文本属性
 * @param window 目标窗口
 * @param property 属性名
 * @param out 返回的信息使用后记得使用 free 释放
 */
void xwindow_get_text_property(xcb_window_t window, xcb_atom_t property,
                               char **out) {
  if (out == nullptr) return;

  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    wm.xcb_conn, false, window, property, XCB_ATOM_ANY, 0, UINT32_MAX);
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(wm.xcb_conn, cookie, nullptr);
  if (!reply) return;

  int length = xcb_get_property_value_length(reply);
  char *value = xcb_get_property_value(reply);

  if (reply &&
      (reply->type == XCB_ATOM_STRING || reply->type == UTF8_STRING ||
       reply->type == COMPOUND_TEXT) &&
      reply->format == 8 && length &&
      (*out == nullptr || strncmp(*out, value, length) != 0)) {
    if (*out) p_delete(out);
    *out = p_new(char, length + 1);
    memcpy(*out, value, length);
    (*out)[length] = '\0';
  }

  p_delete(&reply);
}
