#include "tray.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#include "atoms-extern.h"
#include "base.h"
#include "monitor.h"
#include "utils.h"
#include "wm.h"
#include "xwindow.h"

typedef enum tray_opcode_t {
  TRAY_REQUEST_DOCK = 0,
  TRAY_BEGIN_MESSAGE = 1,
  TRAY_CANCEL_MESSAGE = 2,
} tray_opcode_t;

typedef enum tray_orientation_t {
  TRAY_ORIENTATION_HORIZONTAL = 0,
} tray_orientation_t;

typedef enum xembed_message_t {
  XEMBED_EMBEDDED_NOTIFY = 0,
} xembed_message_t;

enum {
  XEMBED_VERSION = 0,
};

typedef struct tray_icon_t {
  xcb_window_t window;
  uint16_t natural_width;
  uint16_t natural_height;
  uint8_t ignore_unmaps;
  bool mapped;
} tray_icon_t;

typedef struct tray_t {
  bool initialized;
  xcb_atom_t selection_atom;
  xcb_window_t window;
  monitor_t *host_monitor;

  tray_icon_t *icons;
  size_t icon_count;
  size_t icon_capacity;

  uint16_t spacing;
  uint16_t side_padding;
} tray_t;

static tray_t tray = {
  .selection_atom = XCB_ATOM_NONE,
  .window = XCB_WINDOW_NONE,
};

static xcb_atom_t tray_intern_atom(const char *name) {
  if (!name || !name[0]) return XCB_ATOM_NONE;

  xcb_intern_atom_cookie_t cookie =
    xcb_intern_atom(wm.xcb_conn, false, (uint16_t)strlen(name), name);
  xcb_intern_atom_reply_t *reply =
    xcb_intern_atom_reply(wm.xcb_conn, cookie, nullptr);
  if (!reply) return XCB_ATOM_NONE;

  xcb_atom_t atom = reply->atom;
  p_delete(&reply);
  return atom;
}

static tray_icon_t *tray_get_icon(xcb_window_t window) {
  for (size_t i = 0; i < tray.icon_count; i++) {
    tray_icon_t *icon = &tray.icons[i];
    if (icon->window == window) return icon;
  }

  return nullptr;
}

static size_t tray_find_icon_index(xcb_window_t window) {
  for (size_t i = 0; i < tray.icon_count; i++) {
    if (tray.icons[i].window == window) return i;
  }

  return SIZE_MAX;
}

static uint16_t tray_target_icon_height(void) {
  return MAX((uint16_t)1, wm.bar_height);
}

static uint32_t tray_color_component_u16(double channel) {
  if (channel < 0.0) channel = 0.0;
  if (channel > 1.0) channel = 1.0;

  uint32_t value = (uint32_t)(channel * 65535.0 + 0.5);
  return MIN(value, 65535u);
}

static void tray_get_icon_layout(const tray_icon_t *icon, uint16_t *width,
                                 uint16_t *height) {
  uint32_t target_height = tray_target_icon_height();
  uint32_t natural_width = icon->natural_width;
  uint32_t natural_height = icon->natural_height;

  if (natural_width == 0) natural_width = target_height;
  if (natural_height == 0) natural_height = target_height;
  if (natural_height == 0) natural_height = 1;

  uint32_t scaled_width =
    (natural_width * target_height + natural_height / 2) / natural_height;
  if (scaled_width == 0) scaled_width = 1;

  if (width) *width = (uint16_t)MIN((uint32_t)UINT16_MAX, scaled_width);
  if (height) *height = (uint16_t)MIN((uint32_t)UINT16_MAX, target_height);
}

static uint16_t tray_compute_width(void) {
  if (!tray.initialized || tray.host_monitor == nullptr ||
      tray.icon_count == 0) {
    return 0;
  }

  uint32_t width = (uint32_t)tray.side_padding * 2;
  for (size_t i = 0; i < tray.icon_count; i++) {
    uint16_t icon_width = 0;
    tray_get_icon_layout(&tray.icons[i], &icon_width, nullptr);
    width += icon_width;
  }
  if (tray.icon_count > 1) {
    width += (uint32_t)(tray.icon_count - 1) * tray.spacing;
  }

  return (uint16_t)MIN((uint32_t)UINT16_MAX, width);
}

static void tray_send_xembed_message(xcb_window_t window, uint32_t message,
                                     uint32_t detail, uint32_t data1,
                                     uint32_t data2) {
  xcb_client_message_event_t ev;
  p_clear(&ev, 1);
  ev.response_type = XCB_CLIENT_MESSAGE;
  ev.format = 32;
  ev.window = window;
  ev.type = _XEMBED;
  ev.data.data32[0] = XCB_CURRENT_TIME;
  ev.data.data32[1] = message;
  ev.data.data32[2] = detail;
  ev.data.data32[3] = data1;
  ev.data.data32[4] = data2;

  xcb_send_event(wm.xcb_conn, false, window, XCB_EVENT_MASK_NO_EVENT,
                 (char *)&ev);
}

static void tray_select_icon_events(xcb_window_t window) {
  xcb_params_cw_t params = {
    .back_pixel = wm.color_set.bar_bg.argb,
    .border_pixel = 0,
    .event_mask =
      XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
  };
  uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
  xcb_aux_change_window_attributes(wm.xcb_conn, window, mask, &params);
  xcb_clear_area(wm.xcb_conn, false, window, 0, 0, 0, 0);
}

static void tray_release_icon_window(tray_icon_t *icon) {
  if (!icon || icon->window == XCB_WINDOW_NONE) return;

  xcb_change_save_set(wm.xcb_conn, XCB_SET_MODE_DELETE, icon->window);
  xcb_params_cw_t params = {
    .event_mask = XCB_EVENT_MASK_NO_EVENT,
    .border_pixel = 0,
  };
  xcb_aux_change_window_attributes(wm.xcb_conn, icon->window,
                                   XCB_CW_EVENT_MASK | XCB_CW_BORDER_PIXEL,
                                   &params);
  xcb_reparent_window(wm.xcb_conn, icon->window, wm.screen->root, 0, 0);
}

static void tray_layout_icons(int16_t right_edge) {
  if (!tray.initialized || tray.window == XCB_WINDOW_NONE ||
      tray.host_monitor == nullptr) {
    return;
  }

  uint16_t tray_width = tray_compute_width();
  if (tray_width == 0) {
    xcb_unmap_window(wm.xcb_conn, tray.window);
    return;
  }

  int32_t tray_x = (int32_t)right_edge - tray_width;
  if (tray_x < 0) tray_x = 0;

  xcb_configure_window_value_list_t tray_values = {
    .x = (int16_t)tray_x,
    .y = 0,
    .width = tray_width,
    .height = wm.bar_height,
  };
  uint16_t tray_mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                       XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
  xcb_configure_window_aux(wm.xcb_conn, tray.window, tray_mask, &tray_values);
  xcb_map_window(wm.xcb_conn, tray.window);

  int32_t x = tray.side_padding;
  for (size_t i = 0; i < tray.icon_count; i++) {
    tray_icon_t *icon = &tray.icons[i];
    uint16_t icon_width = 0;
    uint16_t icon_height = 0;
    tray_get_icon_layout(icon, &icon_width, &icon_height);

    int32_t y = ((int32_t)wm.bar_height - icon_height) / 2;
    if (y < 0) y = 0;

    xcb_configure_window_value_list_t icon_values = {
      .x = (int16_t)x,
      .y = (int16_t)y,
      .width = icon_width,
      .height = icon_height,
      .border_width = 0,
    };
    uint16_t icon_mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                         XCB_CONFIG_WINDOW_BORDER_WIDTH;
    xcb_configure_window_aux(wm.xcb_conn, icon->window, icon_mask,
                             &icon_values);
    xcb_map_window(wm.xcb_conn, icon->window);
    icon->mapped = true;

    x += icon_width + tray.spacing;
  }
}

static void tray_request_redraw(void) {
  if (!tray.initialized || tray.host_monitor == nullptr) return;

  monitor_draw_bar(tray.host_monitor);
  xcb_flush(wm.xcb_conn);
}

static void tray_remove_icon_by_index(size_t index, bool release_window) {
  if (index >= tray.icon_count) return;

  tray_icon_t icon = tray.icons[index];
  if (release_window) tray_release_icon_window(&icon);

  if (index + 1 < tray.icon_count) {
    memmove(&tray.icons[index], &tray.icons[index + 1],
            sizeof(*tray.icons) * (tray.icon_count - index - 1));
  }
  tray.icon_count--;

  tray_request_redraw();
}

static void tray_add_icon(xcb_window_t window) {
  if (!tray.initialized || window == XCB_WINDOW_NONE ||
      tray_get_icon(window) != nullptr) {
    return;
  }

  if (tray.icon_count == tray.icon_capacity) {
    tray.icon_capacity = (size_t)p_alloc_nr(tray.icon_capacity);
    p_realloc(&tray.icons, tray.icon_capacity);
  }

  xcb_get_geometry_reply_t *geometry = xwindow_get_geometry_reply(window);
  if (!geometry) return;

  tray_icon_t *icon = &tray.icons[tray.icon_count++];
  *icon = (tray_icon_t){
    .window = window,
    .natural_width = geometry->width,
    .natural_height = geometry->height,
    .ignore_unmaps = 1,
    .mapped = true,
  };
  p_delete(&geometry);

  xcb_change_save_set(wm.xcb_conn, XCB_SET_MODE_INSERT, window);
  xcb_reparent_window(wm.xcb_conn, window, tray.window, 0, 0);
  tray_select_icon_events(window);
  tray_send_xembed_message(window, XEMBED_EMBEDDED_NOTIFY, 0, tray.window,
                           XEMBED_VERSION);
  xcb_map_window(wm.xcb_conn, window);

  tray_request_redraw();
}

static bool tray_create_window(void) {
  static xcb_colormap_t colormap = XCB_NONE;
  if (tray.host_monitor == nullptr ||
      tray.host_monitor->bar_window == XCB_NONE) {
    return false;
  }

  visual_t *visual = xwindow_get_xcb_visual(false);
  if (visual == nullptr) return false;

  xcb_visualid_t visual_id = visual->visual->visual_id;
  if (colormap == XCB_NONE) {
    colormap = xcb_generate_id(wm.xcb_conn);
    xcb_create_colormap(wm.xcb_conn, XCB_COLORMAP_ALLOC_NONE, colormap,
                        wm.screen->root, visual_id);
  }

  xcb_window_t window = xcb_generate_id(wm.xcb_conn);
  uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
                        XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  const xcb_create_window_value_list_t value_list = {
    .background_pixel = wm.color_set.bar_bg.argb,
    .border_pixel = 0,
    .event_mask =
      XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
    .colormap = colormap,
  };

  xcb_void_cookie_t cookie = xcb_create_window_aux_checked(
    wm.xcb_conn, visual->depth, window, tray.host_monitor->bar_window, 0, 0, 1,
    wm.bar_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual_id, value_mask,
    &value_list);
  p_delete(&visual);
  if (xcb_request_check(wm.xcb_conn, cookie)) {
    logger("cannot create system tray window\n");
    return false;
  }

  tray.window = window;
  xwindow_set_class_instance(window);
#define NAME APP_NAME "_tray"
  xwindow_set_name_static(window, NAME);
  xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, window, _NET_WM_NAME,
                      UTF8_STRING, 8, sizeof(NAME) - 1, NAME);
#undef NAME

  return true;
}

static bool tray_take_selection_owner(void) {
  xcb_get_selection_owner_cookie_t owner_cookie =
    xcb_get_selection_owner(wm.xcb_conn, tray.selection_atom);
  xcb_get_selection_owner_reply_t *owner_reply =
    xcb_get_selection_owner_reply(wm.xcb_conn, owner_cookie, nullptr);
  if (owner_reply && owner_reply->owner != XCB_WINDOW_NONE) {
    logger("system tray selection is already owned by window %u\n",
           owner_reply->owner);
    p_delete(&owner_reply);
    return false;
  }
  p_delete(&owner_reply);

  xcb_set_selection_owner(wm.xcb_conn, tray.window, tray.selection_atom,
                          XCB_CURRENT_TIME);

  owner_cookie = xcb_get_selection_owner(wm.xcb_conn, tray.selection_atom);
  owner_reply =
    xcb_get_selection_owner_reply(wm.xcb_conn, owner_cookie, nullptr);
  bool success = owner_reply && owner_reply->owner == tray.window;
  p_delete(&owner_reply);

  if (!success) logger("cannot acquire system tray selection\n");
  return success;
}

static void tray_set_properties(void) {
  uint32_t orientation = TRAY_ORIENTATION_HORIZONTAL;
  xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, tray.window,
                      _NET_SYSTEM_TRAY_ORIENTATION, XCB_ATOM_CARDINAL, 32, 1,
                      &orientation);

  const color_t *fg = &wm.color_set.tag_color;
  uint32_t colors[12] = {
    tray_color_component_u16(fg->red),   tray_color_component_u16(fg->green),
    tray_color_component_u16(fg->blue),  tray_color_component_u16(fg->red),
    tray_color_component_u16(fg->green), tray_color_component_u16(fg->blue),
    tray_color_component_u16(fg->red),   tray_color_component_u16(fg->green),
    tray_color_component_u16(fg->blue),  tray_color_component_u16(fg->red),
    tray_color_component_u16(fg->green), tray_color_component_u16(fg->blue),
  };
  xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, tray.window,
                      _NET_SYSTEM_TRAY_COLORS, XCB_ATOM_CARDINAL, 32,
                      countof(colors), colors);
}

static void tray_announce_manager(void) {
  xcb_client_message_event_t ev;
  p_clear(&ev, 1);
  ev.response_type = XCB_CLIENT_MESSAGE;
  ev.window = wm.screen->root;
  ev.format = 32;
  ev.type = MANAGER;
  ev.data.data32[0] = XCB_CURRENT_TIME;
  ev.data.data32[1] = tray.selection_atom;
  ev.data.data32[2] = tray.window;

  xcb_send_event(wm.xcb_conn, false, wm.screen->root,
                 XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *)&ev);
}

void tray_init(void) {
  if (tray.initialized) return;

  tray.host_monitor = wm.monitor_list;
  if (tray.host_monitor == nullptr) return;

  tray.side_padding = 0;
  tray.spacing = 0;

  char selection_name[64];
  snprintf(selection_name, sizeof(selection_name), "_NET_SYSTEM_TRAY_S%d",
           wm.default_screen);
  tray.selection_atom = tray_intern_atom(selection_name);
  if (tray.selection_atom == XCB_ATOM_NONE) {
    logger("cannot intern tray selection atom %s\n", selection_name);
    return;
  }

  if (!tray_create_window()) return;
  if (!tray_take_selection_owner()) {
    tray_cleanup();
    return;
  }

  tray.initialized = true;
  tray_set_properties();
  tray_announce_manager();
  tray_layout_icons((int16_t)tray.host_monitor->geometry.width);
  xcb_flush(wm.xcb_conn);
}

void tray_cleanup(void) {
  for (size_t i = 0; i < tray.icon_count; i++) {
    tray_release_icon_window(&tray.icons[i]);
  }
  p_delete(&tray.icons);
  tray.icon_count = 0;
  tray.icon_capacity = 0;

  if (tray.selection_atom != XCB_ATOM_NONE) {
    xcb_set_selection_owner(wm.xcb_conn, XCB_WINDOW_NONE, tray.selection_atom,
                            XCB_CURRENT_TIME);
  }

  if (tray.window != XCB_WINDOW_NONE) {
    xcb_destroy_window(wm.xcb_conn, tray.window);
  }

  tray = (tray_t){
    .selection_atom = XCB_ATOM_NONE,
    .window = XCB_WINDOW_NONE,
  };
}

bool tray_handle_client_message(xcb_client_message_event_t *ev) {
  if (!tray.initialized) return false;

  if (ev->type != _NET_SYSTEM_TRAY_OPCODE) return false;
  if (ev->window != tray.window && ev->window != wm.screen->root) return false;

  switch (ev->data.data32[1]) {
    case TRAY_REQUEST_DOCK:
      tray_add_icon((xcb_window_t)ev->data.data32[2]);
      break;
    case TRAY_BEGIN_MESSAGE:
    case TRAY_CANCEL_MESSAGE:
    default:
      break;
  }

  xcb_flush(wm.xcb_conn);
  return true;
}

bool tray_handle_configure_request(xcb_configure_request_event_t *ev) {
  if (!tray.initialized) return false;

  if (ev->window == tray.window) {
    tray_layout_icons((int16_t)tray.host_monitor->geometry.width);
    xcb_flush(wm.xcb_conn);
    return true;
  }

  tray_icon_t *icon = tray_get_icon(ev->window);
  if (!icon) return false;

  if ((ev->value_mask & XCB_CONFIG_WINDOW_WIDTH) && ev->width > 0) {
    icon->natural_width = ev->width;
  }
  if ((ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT) && ev->height > 0) {
    icon->natural_height = ev->height;
  }

  tray_request_redraw();
  return true;
}

bool tray_handle_map_request(xcb_map_request_event_t *ev) {
  if (!tray.initialized) return false;

  if (ev->window == tray.window) {
    tray_layout_icons((int16_t)tray.host_monitor->geometry.width);
    xcb_flush(wm.xcb_conn);
    return true;
  }

  tray_icon_t *icon = tray_get_icon(ev->window);
  if (!icon && ev->parent != tray.window) return false;

  if (icon) icon->mapped = true;
  xcb_map_window(wm.xcb_conn, ev->window);
  tray_layout_icons((int16_t)tray.host_monitor->geometry.width);
  xcb_flush(wm.xcb_conn);
  return true;
}

bool tray_handle_destroy_notify(xcb_destroy_notify_event_t *ev) {
  if (!tray.initialized) return false;

  if (ev->window == tray.window) {
    tray.window = XCB_WINDOW_NONE;
    tray.initialized = false;
    return true;
  }

  size_t index = tray_find_icon_index(ev->window);
  if (index == SIZE_MAX) return false;

  tray_remove_icon_by_index(index, false);
  return true;
}

bool tray_handle_unmap_notify(xcb_unmap_notify_event_t *ev) {
  if (!tray.initialized) return false;

  tray_icon_t *icon = tray_get_icon(ev->window);
  if (!icon) return ev->window == tray.window;

  if (icon->ignore_unmaps > 0) {
    icon->ignore_unmaps--;
    return true;
  }

  size_t index = tray_find_icon_index(ev->window);
  if (index != SIZE_MAX) tray_remove_icon_by_index(index, true);

  return true;
}

bool tray_handle_property_notify(xcb_property_notify_event_t *ev) {
  if (!tray.initialized) return false;
  return tray_is_window(ev->window);
}

bool tray_handle_selection_clear(xcb_selection_clear_event_t *ev) {
  if (!tray.initialized) return false;
  if (ev->owner != tray.window) return false;
  if (ev->selection != tray.selection_atom) return false;

  monitor_t *host_monitor = tray.host_monitor;
  logger("system tray selection lost\n");
  tray_cleanup();

  if (host_monitor != nullptr) monitor_draw_bar(host_monitor);
  xcb_flush(wm.xcb_conn);
  return true;
}

uint16_t tray_get_width(monitor_t *monitor) {
  if (!tray.initialized || monitor == nullptr || monitor != tray.host_monitor) {
    return 0;
  }

  return tray_compute_width();
}

void tray_place(monitor_t *monitor, int16_t right_edge) {
  if (!tray.initialized || monitor == nullptr || monitor != tray.host_monitor) {
    return;
  }

  tray_layout_icons(right_edge);
}

bool tray_is_window(xcb_window_t window) {
  if (window == XCB_WINDOW_NONE) return false;
  if (tray.window == window) return true;
  return tray_get_icon(window) != nullptr;
}
