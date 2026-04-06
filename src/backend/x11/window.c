#include "window.h"

#include <stddef.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "base/memory.h"
#include "internal.h"

static char *window_get_text_property(backend_t *backend, xcb_window_t window,
                                      xcb_atom_t property) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    backend->conn, false, window, property, XCB_ATOM_ANY, 0, UINT32_MAX);
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(backend->conn, cookie, nullptr);
  if (!reply) return nullptr;

  int length = xcb_get_property_value_length(reply);
  char *value = (char *)xcb_get_property_value(reply);

  char *text = nullptr;
  if (length && reply->format == 8 &&
      (reply->type == XCB_ATOM_STRING ||
       reply->type == backend->atoms.UTF8_STRING ||
       reply->type == backend->atoms.COMPOUND_TEXT)) {
    text = p_strndup(value, (size_t)length);
  }

  p_delete(&reply);
  return text;
}

char *window_get_role(backend_t *backend, xcb_window_t window) {
  xcb_atom_t property = backend->atoms.WM_WINDOW_ROLE;
  return window_get_text_property(backend, window, property);
}

char *window_get_title(backend_t *backend, xcb_window_t window) {
  xcb_atom_t property = backend->atoms._NET_WM_NAME;
  char *name = window_get_text_property(backend, window, property);
  if (!name) {
    property = backend->atoms.WM_NAME;
    name = window_get_text_property(backend, window, property);
  }
  return name;
}

void window_get_class(backend_t *backend, xcb_window_t window, char **class_out,
                      char **instance_out) {
  xcb_icccm_get_wm_class_reply_t prop;
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_class_unchecked(backend->conn, window);
  if (!xcb_icccm_get_wm_class_reply(backend->conn, cookie, &prop, nullptr)) {
    return;
  }

  if (class_out) *class_out = p_strdup_nullable(prop.class_name);
  if (instance_out) *instance_out = p_strdup_nullable(prop.instance_name);

  xcb_icccm_get_wm_class_reply_wipe(&prop);
}
