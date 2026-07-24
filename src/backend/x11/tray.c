#include "backend/x11/tray.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>
#include <zdwm/types.h>

#include "backend/x11/window.h"
#include "base/array.h"
#include "base/log.h"
#include "base/macros.h"
#include "base/memory.h"
#include "interface/tray.h"
#include "interface/types.h"
#include "internal.h"

typedef enum tray_opcode_t {
  SYSTEM_TRAY_REQUEST_DOCK = 0,
  SYSTEM_TRAY_BEGIN_MESSAGE = 1,
  SYSTEM_TRAY_CANCEL_MESSAGE = 2,
} tray_opcode_t;

typedef enum xembed_message_t {
  XEMBED_EMBEDDED_NOTIFY = 0,
} xembed_message_t;

static constexpr auto XEMBED_VERSION = 0;

typedef struct tray_icon_t {
  xcb_window_t window;
  uint16_t natural_width;
  uint16_t natural_height;
} tray_icon_t;

typedef struct tray_host_t {
  bool enabled;
  bool initialized;
  xcb_atom_t selection_atom;
  xcb_window_t host_window;
  xcb_window_t container;
  int32_t icon_size;

  tray_icon_t *icons;
  size_t icon_count;
  size_t icon_capacity;

  tray_icons_change_cb_t icon_change_cb;
  void *cb_user_data;
} tray_host_t;

static tray_host_t *get_tray_host(void *handle) {
  backend_t *backend = handle;
  if (!backend) return nullptr;

  auto tray = backend->tray;
  if (!tray || !tray->enabled) return nullptr;

  return tray;
}

static tray_icon_t *tray_get_icon(tray_host_t *tray, xcb_window_t window) {
  for (size_t i = 0; i < tray->icon_count; ++i) {
    auto icon = &tray->icons[i];
    if (icon->window == window) return icon;
  }

  return nullptr;
}

static inline void tray_dispatch_callback(tray_host_t *tray) {
  assert(tray);
  if (tray->icon_change_cb) tray->icon_change_cb(tray->cb_user_data);
}

static void tray_release_icon_window(backend_t *backend, tray_icon_t *icon) {
  if (!icon || icon->window == XCB_WINDOW_NONE) return;

  auto tray = get_tray_host(backend);
  if (!tray || !tray->initialized) return;

  auto conn = backend->conn;
  auto root = backend->screen->root;
  auto window = icon->window;
  auto mask = XCB_CW_EVENT_MASK;
  xcb_params_cw_t params = {.event_mask = XCB_EVENT_MASK_NO_EVENT};
  xcb_change_save_set(conn, XCB_SET_MODE_DELETE, window);
  xcb_aux_change_window_attributes(conn, window, mask, &params);
  xcb_reparent_window(conn, window, root, 0, 0);
}

static window_id_t tray_host_window(void *handle) {
  auto tray = get_tray_host(handle);
  if (!tray) return ZDWM_WINDOW_ID_INVALID;

  return tray->host_window;
}

static size_t tray_icon_count(void *handle) {
  auto tray = get_tray_host(handle);

  return tray ? tray->icon_count : 0;
}

static int32_t tray_icon_size(void *handle) {
  auto tray = get_tray_host(handle);

  return tray ? tray->icon_size : 0;
}

static void tray_place(void *handle, int32_t x) {
  auto tray = get_tray_host(handle);
  if (!tray) return;

  auto conn = ((backend_t *)handle)->conn;
  uint16_t value_mask = XCB_CONFIG_WINDOW_X;
  xcb_configure_window_value_list_t value_list = {.x = x};
  xcb_configure_window_aux(conn, tray->container, value_mask, &value_list);
}

static void
tray_set_listeners(void *handle, tray_icons_change_cb_t cb, void *user_data) {
  auto tray = get_tray_host(handle);
  if (!tray) return;

  tray->icon_change_cb = cb;
  tray->cb_user_data = user_data;
}

const tray_api_t tray_api = {
  .host_window = tray_host_window,
  .icon_count = tray_icon_count,
  .icon_size = tray_icon_size,
  .place = tray_place,
  .set_listener = tray_set_listeners,
};

static bool tray_intern_selection_atom(tray_host_t *tray, backend_t *backend) {
  auto conn = backend->conn;
  auto screenp = backend->screenp;

  char selection_name[64] = {0};
  auto atom_name_format = "_NET_SYSTEM_TRAY_S%d";
  snprintf(selection_name, sizeof(selection_name), atom_name_format, screenp);

  uint16_t name_length = strlen(selection_name);
  auto cookie = xcb_intern_atom(conn, false, name_length, selection_name);
  auto reply = xcb_intern_atom_reply(conn, cookie, nullptr);
  if (!reply) {
    warn("cannot intern tray selection atom: %s", selection_name);
    return false;
  }

  tray->selection_atom = reply->atom;
  p_delete(&reply);
  return true;
}

typedef struct tray_host_window_t {
  xcb_window_t window;
  int32_t width;
  int32_t height;
} tray_host_window_t;

static bool tray_create_container_window(
  tray_host_t *tray,
  backend_t *backend,
  tray_host_window_t host_window
) {
  auto conn = backend->conn;
  auto window_id = xcb_generate_id(conn);

  xcb_create_window_value_list_t value_list = {
    .override_redirect = true,
    .event_mask =
      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_STRUCTURE_NOTIFY
  };
  auto cookie = xcb_create_window_aux_checked(
    conn,
    XCB_COPY_FROM_PARENT,
    window_id,
    host_window.window,
    (int16_t)host_window.width,
    0,
    1,
    (uint16_t)MAX(1, host_window.height),
    0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    XCB_COPY_FROM_PARENT,
    XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK,
    &value_list
  );

  if (xcb_request_check(conn, cookie)) {
    warn("cannot create system tray container window: 0x%x", window_id);
    return false;
  }

  tray->container = window_id;

  window_set_class_instance(conn, window_id);

#define TRAY_CONTAINER_WINDOW_NAME APP_NAME "_tray"
  window_set_name_static(conn, window_id, TRAY_CONTAINER_WINDOW_NAME);
  auto name = TRAY_CONTAINER_WINDOW_NAME;
  auto name_len = sizeof(TRAY_CONTAINER_WINDOW_NAME) - 1;
#undef TRAY_CONTAINER_WINDOW_NAME

  uint8_t mode = XCB_PROP_MODE_REPLACE;
  auto atoms = &backend->atoms;
  auto prop = atoms->_NET_WM_NAME;
  auto type = atoms->UTF8_STRING;
  xcb_change_property(conn, mode, window_id, prop, type, 8, name_len, name);

  return true;
}

static bool tray_take_selection_owner(tray_host_t *tray, backend_t *backend) {
  auto conn = backend->conn;
  auto atom = tray->selection_atom;
  auto cookie = xcb_get_selection_owner(conn, atom);
  auto reply = xcb_get_selection_owner_reply(conn, cookie, nullptr);
  if (reply && reply->owner != XCB_WINDOW_NONE) {
    warn(
      "system tray selection is already owned by window: 0x%x",
      reply->owner
    );
    p_delete(&reply);
    return false;
  }

  p_delete(&reply);

  auto owner = tray->container;
  xcb_set_selection_owner(conn, owner, atom, XCB_CURRENT_TIME);

  cookie = xcb_get_selection_owner(conn, atom);
  reply = xcb_get_selection_owner_reply(conn, cookie, nullptr);
  bool success = reply && reply->owner == owner;
  p_delete(&reply);

  if (!success) warn("cannot acquire system tray selection");
  return success;
}

static void tray_broadcast_manager(tray_host_t *tray, backend_t *backend) {
  auto conn = backend->conn;
  auto root = backend->screen->root;

  xcb_client_message_event_t event = {
    .response_type = XCB_CLIENT_MESSAGE,
    .window = root,
    .format = 32,
    .type = backend->atoms.MANAGER,
    .data.data32 = {
      [0] = XCB_CURRENT_TIME,
      [1] = tray->selection_atom,
      [2] = tray->container,
    },
  };
  uint32_t event_mask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
  xcb_send_event(conn, false, root, event_mask, (char *)&event);
}

void tray_init(
  backend_t *backend,
  xcb_window_t host_window,
  int32_t host_width,
  int32_t host_height
) {
  auto tray = get_tray_host(backend);
  if (!tray) {
    tray = p_new(tray_host_t, 1);
    backend->tray = tray;
  }

  if (tray->initialized) return;

  if (!tray_intern_selection_atom(tray, backend)) return;

  tray_host_window_t host_window_params = {
    .window = host_window,
    .width = host_width,
    .height = host_height
  };
  if (!tray_create_container_window(tray, backend, host_window_params)) return;
  if (!tray_take_selection_owner(tray, backend)) {
    tray_cleanup(backend);
    return;
  }

  tray->enabled = true;
  tray->initialized = true;

  tray_broadcast_manager(tray, backend);
  xcb_flush(backend->conn);
}

void tray_cleanup(backend_t *backend) {
  auto tray = backend->tray;
  if (!tray) return;

  for (size_t i = 0; i < tray->icon_count; ++i) {
    tray_release_icon_window(backend, &tray->icons[i]);
  }
  p_delete(&tray->icons);
  tray->icon_count = 0;
  tray->icon_capacity = 0;

  auto conn = backend->conn;

  auto atom = tray->selection_atom;
  if (atom != XCB_ATOM_NONE) {
    xcb_set_selection_owner(conn, XCB_WINDOW_NONE, atom, XCB_CURRENT_TIME);
  }

  if (tray->container != XCB_WINDOW_NONE) {
    xcb_destroy_window(conn, tray->container);
  }

  p_clear(backend->tray, 1);
  p_delete(&backend->tray);
}

static void
tray_select_icon_events(xcb_connection_t *conn, xcb_window_t window) {
  xcb_params_cw_t params = {
    .event_mask =
      XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE,
  };
  uint32_t mask = XCB_CW_EVENT_MASK;
  xcb_aux_change_window_attributes(conn, window, mask, &params);
  xcb_clear_area(conn, false, window, 0, 0, 0, 0);
}

/* reference:
 * https://specifications.freedesktop.org/xembed/latest-single/#id-1.7.4 */
typedef struct xembed_message_data_t {
  uint32_t message; /* message opcode */
  uint32_t detail;  /* message detail */
  uint32_t data1;   /* message data 1 */
  uint32_t data2;   /* message data 2 */
} xembed_message_data_t;

static void tray_send_xembed_message(
  backend_t *backend,
  xcb_window_t window,
  xembed_message_data_t message
) {
  auto conn = backend->conn;
  auto message_type = backend->atoms._XEMBED;

  xcb_client_message_event_t event = {
    .response_type = XCB_CLIENT_MESSAGE,
    .format = 32,
    .window = window,
    .type = message_type,
    .data.data32 = {
      [0] = XCB_CURRENT_TIME,
      [1] = message.message,
      [2] = message.detail,
      [3] = message.data1,
      [4] = message.data2,
    },
  };
  uint32_t event_mask = XCB_EVENT_MASK_NO_EVENT;
  xcb_send_event(conn, false, window, event_mask, (char *)&event);
}

static void tray_add_icon(backend_t *backend, xcb_window_t window) {
  if (window == XCB_WINDOW_NONE) return;

  auto tray = get_tray_host(backend);
  if (!tray || !tray->initialized || tray_get_icon(tray, window)) return;

  rect_t geometry = {0};
  if (!window_get_geometry(backend, window, &geometry)) return;

  auto icon = array_push(tray->icons, tray->icon_count, tray->icon_capacity);
  *icon = (tray_icon_t){
    .window = window,
    .natural_width = geometry.width,
    .natural_height = geometry.height
  };

  auto conn = backend->conn;
  auto container = tray->container;
  xembed_message_data_t message = {
    .message = XEMBED_EMBEDDED_NOTIFY,
    .detail = 0,
    .data1 = container,
    .data2 = XEMBED_VERSION,
  };
  xcb_change_save_set(conn, XCB_SET_MODE_INSERT, window);
  xcb_reparent_window(conn, window, tray->container, 0, 0);
  tray_select_icon_events(conn, window);
  tray_send_xembed_message(backend, window, message);
  xcb_map_window(conn, window);
  tray_dispatch_callback(tray);
}

bool tray_handle_client_message(
  backend_t *backend,
  const xcb_client_message_event_t *ev
) {
  auto tray = get_tray_host(backend);
  if (!tray || !tray->enabled || !tray->initialized) return false;
  if (ev->window != tray->container && ev->window != backend->screen->root) {
    return false;
  }

  auto opcode = ev->data.data32[1];
  switch (opcode) {
  case SYSTEM_TRAY_REQUEST_DOCK:
    auto window = ev->data.data32[2];
    tray_add_icon(backend, window);
    break;
  case SYSTEM_TRAY_BEGIN_MESSAGE:
  case SYSTEM_TRAY_CANCEL_MESSAGE:
  default:
    break;
  }

  xcb_flush(backend->conn);

  return true;
}
