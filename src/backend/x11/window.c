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

xcb_window_t window_get_transient_for(backend_t *backend, xcb_window_t window) {
  xcb_connection_t *conn = backend->conn;
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_transient_for_unchecked(conn, window);
  xcb_window_t trans_for = XCB_WINDOW_NONE;
  if (xcb_icccm_get_wm_transient_for_reply(conn, cookie, &trans_for, nullptr)) {
    return trans_for;
  }

  return XCB_WINDOW_NONE;
}

xcb_get_window_attributes_reply_t *window_get_attributes(backend_t *backend,
                                                         xcb_window_t window) {
  xcb_get_window_attributes_cookie_t cookie =
    xcb_get_window_attributes(backend->conn, window);
  return xcb_get_window_attributes_reply(backend->conn, cookie, nullptr);
}

bool window_get_skip_taskbar(backend_t *backend, xcb_window_t window) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    backend->conn, false, window, backend->atoms._NET_WM_STATE, XCB_ATOM_ATOM,
    0, UINT32_MAX);
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(backend->conn, cookie, nullptr);
  if (!reply) return false;

  bool found = false;
  if (reply->type == XCB_ATOM_ATOM && reply->format == 32) {
    xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
    uint32_t len = reply->value_len;
    for (uint32_t i = 0; i < len; i++) {
      if (atoms[i] == backend->atoms._NET_WM_STATE_SKIP_TASKBAR) {
        found = true;
        break;
      }
    }
  }

  p_delete(&reply);
  return found;
}

bool window_get_urgency(backend_t *backend, xcb_window_t window) {
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_hints_unchecked(backend->conn, window);
  xcb_icccm_wm_hints_t hints;
  xcb_icccm_get_wm_hints_reply(backend->conn, cookie, &hints, nullptr);
  return (bool)xcb_icccm_wm_hints_get_urgency(&hints);
}

static xcb_atom_t *window_get_atom_array(backend_t *backend,
                                         xcb_window_t window,
                                         xcb_atom_t property,
                                         uint32_t *out_len) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    backend->conn, false, window, property, XCB_ATOM_ATOM, 0, UINT32_MAX);
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(backend->conn, cookie, nullptr);
  if (!reply) return nullptr;

  xcb_atom_t *result = nullptr;
  if (reply->type == XCB_ATOM_ATOM && reply->format == 32 && reply->value_len) {
    *out_len = reply->value_len;
    result =
      p_copy((xcb_atom_t *)xcb_get_property_value(reply), reply->value_len);
  }

  p_delete(&reply);
  return result;
}

static window_type_t atom_to_window_type(const atoms_t *atoms,
                                         xcb_atom_t atom) {
  if (atom == atoms->_NET_WM_WINDOW_TYPE_DESKTOP)
    return ZDWM_WINDOW_TYPE_DESKTOP;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_DOCK) return ZDWM_WINDOW_TYPE_DOCK;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_TOOLBAR)
    return ZDWM_WINDOW_TYPE_TOOLBAR;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_DIALOG) return ZDWM_WINDOW_TYPE_DIALOG;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_UTILITY)
    return ZDWM_WINDOW_TYPE_UTILITY;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_SPLASH) return ZDWM_WINDOW_TYPE_SPLASH;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_MENU) return ZDWM_WINDOW_TYPE_MENU;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
    return ZDWM_WINDOW_TYPE_DROPDOWN_MENU;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_POPUP_MENU)
    return ZDWM_WINDOW_TYPE_POPUP_MENU;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_TOOLTIP)
    return ZDWM_WINDOW_TYPE_TOOLTIP;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_COMBO) return ZDWM_WINDOW_TYPE_COMBO;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_DND) return ZDWM_WINDOW_TYPE_DND;
  if (atom == atoms->_NET_WM_WINDOW_TYPE_NOTIFICATION)
    return ZDWM_WINDOW_TYPE_NOTIFICATION;
  return ZDWM_WINDOW_TYPE_NORMAL;
}

bool window_get_types(backend_t *backend, xcb_window_t window,
                      window_type_t **types, size_t *count) {
  if (!types || !count) return false;

  uint32_t atom_count = 0;
  xcb_atom_t *atoms = window_get_atom_array(
    backend, window, backend->atoms._NET_WM_WINDOW_TYPE, &atom_count);
  if (!atoms) {
    *types = nullptr;
    *count = 0;
    return false;
  }

  *count = atom_count;
  *types = p_new(window_type_t, atom_count);
  for (uint32_t i = 0; i < atom_count; i++) {
    (*types)[i] = atom_to_window_type(&backend->atoms, atoms[i]);
  }

  p_delete(&atoms);
  return true;
}

static window_state_t atom_to_window_state(const atoms_t *atoms,
                                           xcb_atom_t atom) {
  if (atom == atoms->_NET_WM_STATE_FULLSCREEN)
    return ZDWM_WINDOW_STATE_FULLSCREEN;
  if (atom == atoms->_NET_WM_STATE_ABOVE) return ZDWM_WINDOW_STATE_ABOVE;
  if (atom == atoms->_NET_WM_STATE_STICKY) return ZDWM_WINDOW_STATE_STICKY;
  if (atom == atoms->_NET_WM_STATE_MODAL) return ZDWM_WINDOW_STATE_MODAL;
  return (window_state_t)-1;
}

bool window_get_states(backend_t *backend, xcb_window_t window,
                       window_state_t **states, size_t *count) {
  if (!states || !count) return false;

  uint32_t atom_count = 0;
  xcb_atom_t *atoms = window_get_atom_array(
    backend, window, backend->atoms._NET_WM_STATE, &atom_count);
  if (!atoms) {
    *states = nullptr;
    *count = 0;
    return false;
  }

  /* 只保留已知 state，跳过未识别的 atom */
  window_state_t *buf = p_new(window_state_t, atom_count);
  size_t valid = 0;
  for (uint32_t i = 0; i < atom_count; i++) {
    window_state_t s = atom_to_window_state(&backend->atoms, atoms[i]);
    if (s != (window_state_t)-1) buf[valid++] = s;
  }

  p_delete(&atoms);

  if (valid == 0) {
    p_delete(&buf);
    *states = nullptr;
    *count = 0;
    return false;
  }

  *states = buf;
  *count = valid;
  return true;
}
