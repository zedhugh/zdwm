#include "window.h"

#include <stddef.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "base/array.h"
#include "base/macros.h"
#include "base/memory.h"
#include "core/backend.h"
#include "core/types.h"
#include "internal.h"

static char *window_get_text_property(
  backend_t *backend,
  xcb_window_t window,
  xcb_atom_t property
) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    backend->conn,
    false,
    window,
    property,
    XCB_ATOM_ANY,
    0,
    UINT32_MAX
  );
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(backend->conn, cookie, nullptr);
  if (!reply) return nullptr;

  int length  = xcb_get_property_value_length(reply);
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
  char *name          = window_get_text_property(backend, window, property);
  if (!name) {
    property = backend->atoms.WM_NAME;
    name     = window_get_text_property(backend, window, property);
  }
  return name;
}

void window_get_class(
  backend_t *backend,
  xcb_window_t window,
  char **class_out,
  char **instance_out
) {
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

xcb_get_window_attributes_reply_t *
window_get_attributes(backend_t *backend, xcb_window_t window) {
  xcb_get_window_attributes_cookie_t cookie =
    xcb_get_window_attributes(backend->conn, window);
  return xcb_get_window_attributes_reply(backend->conn, cookie, nullptr);
}

static window_type_t
atom_to_window_type(const atoms_t *atoms, xcb_atom_t atom) {
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

bool window_get_types(
  backend_t *backend,
  xcb_window_t window,
  window_type_t **types,
  size_t *count
) {
  uint32_t atom_count = 0;
  xcb_atom_t *atoms   = nullptr;
  if (!window_get_atom_array(
        backend,
        window,
        backend->atoms._NET_WM_WINDOW_TYPE,
        &atoms,
        &atom_count
      )) {
    return false;
  }
  if (!atoms) {
    *types = nullptr;
    *count = 0;
    return true;
  }

  *count = atom_count;
  *types = p_new(window_type_t, atom_count);
  for (uint32_t i = 0; i < atom_count; i++) {
    (*types)[i] = atom_to_window_type(&backend->atoms, atoms[i]);
  }

  p_delete(&atoms);
  return true;
}

bool window_get_fixed_size(backend_t *backend, xcb_window_t window, bool *out) {
  xcb_size_hints_t hints = {0};
  xcb_connection_t *conn = backend->conn;
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_normal_hints_unchecked(conn, window);
  if (!xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, nullptr)) {
    return false;
  }

  if ((hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) &&
      (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)) {
    *out = hints.min_width == hints.max_width &&
           hints.min_height == hints.max_height;
  } else {
    *out = false;
  }
  return true;
}

bool window_get_geometry(backend_t *backend, xcb_window_t window, rect_t *out) {
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(backend->conn, window);
  xcb_get_geometry_reply_t *reply =
    xcb_get_geometry_reply(backend->conn, cookie, nullptr);
  if (!reply) return false;

  out->x      = reply->x;
  out->y      = reply->y;
  out->width  = reply->width;
  out->height = reply->height;

  p_delete(&reply);
  return true;
}

bool window_get_atom_array(
  backend_t *backend,
  xcb_window_t window,
  xcb_atom_t property,
  xcb_atom_t **out_atoms,
  uint32_t *out_len
) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    backend->conn,
    false,
    window,
    property,
    XCB_ATOM_ATOM,
    0,
    UINT32_MAX
  );
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(backend->conn, cookie, nullptr);
  if (!reply) return false;

  if (reply->type == XCB_ATOM_ATOM && reply->format == 32 && reply->value_len) {
    *out_len = reply->value_len;
    *out_atoms =
      p_copy((xcb_atom_t *)xcb_get_property_value(reply), reply->value_len);
  } else {
    *out_len   = 0;
    *out_atoms = nullptr;
  }

  p_delete(&reply);
  return true;
}

bool window_get_wm_hints(
  backend_t *backend,
  xcb_window_t window,
  xcb_icccm_wm_hints_t *out
) {
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_hints_unchecked(backend->conn, window);
  if (!xcb_icccm_get_wm_hints_reply(backend->conn, cookie, out, nullptr)) {
    p_clear(out, 1);
    return false;
  }
  return true;
}

window_state_t atom_to_window_state(const atoms_t *atoms, xcb_atom_t atom) {
  if (atom == atoms->_NET_WM_STATE_FULLSCREEN)
    return ZDWM_WINDOW_STATE_FULLSCREEN;
  if (atom == atoms->_NET_WM_STATE_ABOVE) return ZDWM_WINDOW_STATE_ABOVE;
  if (atom == atoms->_NET_WM_STATE_STICKY) return ZDWM_WINDOW_STATE_STICKY;
  if (atom == atoms->_NET_WM_STATE_MODAL) return ZDWM_WINDOW_STATE_MODAL;
  return (window_state_t)-1;
}

static bool
window_send_event(backend_t *backend, xcb_window_t window, xcb_atom_t atom) {
  bool exist = false;

  xcb_connection_t *conn = backend->conn;
  auto WM_PROTOCOLS      = backend->atoms.WM_PROTOCOLS;

  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_protocols_unchecked(conn, window, WM_PROTOCOLS);
  xcb_icccm_get_wm_protocols_reply_t reply = {0};
  if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &reply, nullptr)) {
    for (uint32_t i = 0; !exist && i < reply.atoms_len; ++i) {
      exist = reply.atoms[i] == atom;
    }
    xcb_icccm_get_wm_protocols_reply_wipe(&reply);
  }

  if (exist) {
    xcb_client_message_event_t ev = {
      .response_type = XCB_CLIENT_MESSAGE,
      .format        = 32,
      .window        = window,
      .type          = WM_PROTOCOLS,
      .data.data32   = {atom, XCB_CURRENT_TIME},
    };
    xcb_send_event(conn, false, window, XCB_EVENT_MASK_NO_EVENT, (char *)&ev);
  }

  return exist;
}

void window_takefocus(backend_t *backend, xcb_window_t window) {
  window_send_event(backend, window, backend->atoms.WM_TAKE_FOCUS);
}

void window_kill(backend_t *backend, xcb_window_t window) {
  if (window_send_event(backend, window, backend->atoms.WM_DELETE_WINDOW)) {
    xcb_kill_client(backend->conn, window);
  }
}

void root_set_event_mask(backend_t *backend) {
  xcb_connection_t *conn                               = backend->conn;
  xcb_window_t root                                    = backend->screen->root;
  xcb_cw_t change_mask                                 = XCB_CW_EVENT_MASK;
  xcb_change_window_attributes_value_list_t value_list = {
    .event_mask =
      XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_KEY_PRESS,
  };
  xcb_change_window_attributes_aux(conn, root, change_mask, &value_list);
}

void window_set_event_mask(xcb_connection_t *conn, xcb_window_t window) {
  xcb_cw_t change_mask                                 = XCB_CW_EVENT_MASK;
  xcb_change_window_attributes_value_list_t value_list = {
    .event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                  XCB_EVENT_MASK_PROPERTY_CHANGE |
                  XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                  XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
  };
  xcb_change_window_attributes_aux(conn, window, change_mask, &value_list);
}

void window_clean_event_mask(xcb_connection_t *conn, xcb_window_t window) {
  xcb_cw_t change_mask                                 = XCB_CW_EVENT_MASK;
  xcb_change_window_attributes_value_list_t value_list = {
    .event_mask = XCB_EVENT_MASK_NO_EVENT,
  };
  xcb_change_window_attributes_aux(conn, window, change_mask, &value_list);
}

void window_set_icccm_wm_state(
  xcb_connection_t *conn,
  xcb_window_t window,
  xcb_icccm_wm_state_t state
) {
  xcb_icccm_wm_hints_t hints;
  xcb_icccm_wm_hints_set_none(&hints);
  switch (state) {
  case XCB_ICCCM_WM_STATE_WITHDRAWN:
    xcb_icccm_wm_hints_set_withdrawn(&hints);
    break;
  case XCB_ICCCM_WM_STATE_NORMAL:
    xcb_icccm_wm_hints_set_normal(&hints);
    break;
  case XCB_ICCCM_WM_STATE_ICONIC:
    xcb_icccm_wm_hints_set_iconic(&hints);
    break;
  }
  xcb_icccm_set_wm_hints(conn, window, &hints);
}

void window_list_push(window_list_t *window_list, xcb_window_t window) {
  xcb_window_t *win =
    array_push(window_list->windows, window_list->count, window_list->capacity);
  *win = window;
}

void window_list_reset(window_list_t *window_list) {
  p_clear(window_list->windows, window_list->count);
  window_list->count = 0;
}

static uint16_t get_xcb_modifier(modifier_mask_t modifiers) {
  typedef struct modifier_map_t {
    modifier_bit_t modifier;
    xcb_mod_mask_t mask;
  } modifier_map_t;
  static constexpr modifier_map_t modifier_map[] = {
    {ZDWM_MOD_SHIFT, XCB_MOD_MASK_SHIFT},
    {ZDWM_MOD_CONTROL, XCB_MOD_MASK_CONTROL},
    {ZDWM_MOD_1, XCB_MOD_MASK_1},
    {ZDWM_MOD_2, XCB_MOD_MASK_2},
    {ZDWM_MOD_3, XCB_MOD_MASK_3},
    {ZDWM_MOD_4, XCB_MOD_MASK_4},
    {ZDWM_MOD_5, XCB_MOD_MASK_5},
  };

  uint16_t mods = ZDWM_MOD_NONE;
  for (size_t i = 0; i < countof(modifier_map); ++i) {
    auto item = &modifier_map[i];
    if (modifiers & item->modifier) mods |= item->mask;
  }
  return mods;
}

void window_grab_keys(
  backend_t *backend,
  xcb_window_t window,
  const key_bind_t *keys,
  size_t count
) {
  auto conn        = backend->conn;
  auto key_symbols = backend->key_symbols;

  xcb_ungrab_key(conn, XCB_GRAB_ANY, window, XCB_MOD_MASK_ANY);

  xcb_grab_mode_t mode = XCB_GRAB_MODE_ASYNC;

  for (size_t i = 0; i < count; ++i) {
    auto key      = &keys[i];
    auto keycodes = xcb_key_symbols_get_keycode(key_symbols, keys->keysym);
    if (!keycodes) continue;

    auto modifiers = get_xcb_modifier(key->modifiers);
    for (auto keycode = keycodes; *keycode != XCB_NO_SYMBOL; ++keycode) {
      xcb_grab_key(conn, true, window, modifiers, *keycode, mode, mode);
    }
    p_delete(&keycodes);
  }
}
