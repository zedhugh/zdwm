#include "core/backend.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>

#include "backend/output_utils.h"
#include "core/types.h"
#include "utils.h"

struct backend_t {
  xcb_connection_t *conn;
  xcb_screen_t *screen;
  int screenp;
  bool have_xfixes;
};

backend_t *backend_create(const char *display_name) {
  int screen_num = 0;
  xcb_connection_t *conn = xcb_connect(display_name, &screen_num);
  int xcb_conn_error = xcb_connection_has_error(conn);
  if (xcb_conn_error) {
    fatal("cannot open display %s, error %d", display_name, xcb_conn_error);
  }

  xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_num);
  if (!screen) fatal("cannot get screen info");
  xcb_window_t root = screen->root;

  uint32_t mask = XCB_CW_EVENT_MASK;
  const xcb_params_cw_t params = {
    .event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
  };
  xcb_void_cookie_t cookie =
    xcb_aux_change_window_attributes_checked(conn, root, mask, &params);
  if (xcb_request_check(conn, cookie)) {
    fatal(
      "another window manager is already running (cannot select "
      "SubstructureRedirect)");
  }

  backend_t *backend = p_new(backend_t, 1);
  backend->conn = conn;
  backend->screen = screen;
  backend->screenp = screen_num;

  xcb_prefetch_extension_data(conn, &xcb_xfixes_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(conn, &xcb_xfixes_id);
  if (query && query->present) {
    xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(
      conn, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
    xcb_xfixes_query_version_reply_t *reply =
      xcb_xfixes_query_version_reply(conn, cookie, nullptr);

    if (reply) {
      backend->have_xfixes = true;
      p_delete(&reply);
    }
  } else {
    backend->have_xfixes = false;
  }

  return backend;
}

void backend_destroy(backend_t *backend) {
  if (!backend) return;

  xcb_disconnect(backend->conn);
  backend->conn = nullptr;
  free(backend);
}

static bool detect_monitor_by_randr(const backend_t *backend,
                                    output_info_t **outputs, size_t *count) {
  xcb_prefetch_extension_data(backend->conn, &xcb_randr_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(backend->conn, &xcb_randr_id);
  if (!query || !query->present) return false;

  xcb_randr_query_version_cookie_t cookie = xcb_randr_query_version(
    backend->conn, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
  xcb_randr_query_version_reply_t *reply =
    xcb_randr_query_version_reply(backend->conn, cookie, nullptr);
  if (!reply) return false;

  /* xcb_randr_get_monitors 接口是 1.5 引入的 */
  uint32_t major_version = reply->major_version;
  uint32_t minor_version = reply->minor_version;
  if (major_version < 1 || (major_version == 1 && minor_version < 5)) {
    p_delete(&reply);
    return false;
  }
  p_delete(&reply);

  xcb_randr_get_monitors_cookie_t monitors_cookie =
    xcb_randr_get_monitors(backend->conn, backend->screen->root, 1);
  xcb_randr_get_monitors_reply_t *monitors_reply =
    xcb_randr_get_monitors_reply(backend->conn, monitors_cookie, nullptr);
  if (!monitors_reply) {
    warn("RandR get monitor failed");
    return false;
  }

  int len = xcb_randr_get_monitors_monitors_length(monitors_reply);
  if (len <= 0) {
    p_delete(&monitors_reply);
    return false;
  }

  output_info_t *output_list = p_new(output_info_t, len);
  xcb_randr_monitor_info_iterator_t iter =
    xcb_randr_get_monitors_monitors_iterator(monitors_reply);
  for (int i = 0; iter.rem; xcb_randr_monitor_info_next(&iter), i++) {
    output_info_t *output = &output_list[i];
    output->geometry.x = iter.data->x;
    output->geometry.y = iter.data->y;
    output->geometry.width = iter.data->width;
    output->geometry.height = iter.data->height;

    xcb_get_atom_name_cookie_t name_cookie =
      xcb_get_atom_name_unchecked(backend->conn, iter.data->name);
    xcb_get_atom_name_reply_t *name_reply =
      xcb_get_atom_name_reply(backend->conn, name_cookie, nullptr);

    if (name_reply) {
      char *name = xcb_get_atom_name_name(name_reply);
      int length = xcb_get_atom_name_name_length(name_reply);
      output->name = strndup(name, length);
      p_delete(&name_reply);
    }
  }

  p_delete(&monitors_reply);

  if (count) *count = (size_t)len;
  if (outputs) {
    *outputs = output_list;
  } else {
    for (size_t i = 0; i < (size_t)len; i++) p_delete(&output_list[i].name);
    p_delete(&output_list);
  }

  return true;
}

static bool detect_monitor_by_xinerama(const backend_t *backend,
                                       output_info_t **outputs, size_t *count) {
  xcb_prefetch_extension_data(backend->conn, &xcb_xinerama_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(backend->conn, &xcb_xinerama_id);
  if (!query || !query->present) return false;

  xcb_xinerama_query_version_cookie_t cookie = xcb_xinerama_query_version(
    backend->conn, XCB_XINERAMA_MAJOR_VERSION, XCB_XINERAMA_MINOR_VERSION);
  xcb_xinerama_query_version_reply_t *version_reply =
    xcb_xinerama_query_version_reply(backend->conn, cookie, nullptr);
  if (!version_reply) return false;
  p_delete(&version_reply);

  xcb_xinerama_is_active_cookie_t active_cookie =
    xcb_xinerama_is_active(backend->conn);
  xcb_xinerama_is_active_reply_t *active_reply =
    xcb_xinerama_is_active_reply(backend->conn, active_cookie, nullptr);
  bool active = active_reply && active_reply->state;
  if (active_reply) p_delete(&active_reply);
  if (!active) return false;

  xcb_xinerama_query_screens_cookie_t screens_cookie =
    xcb_xinerama_query_screens(backend->conn);
  xcb_xinerama_query_screens_reply_t *screens_reply =
    xcb_xinerama_query_screens_reply(backend->conn, screens_cookie, nullptr);

  if (!screens_reply) return false;

  int len = xcb_xinerama_query_screens_screen_info_length(screens_reply);
  if (len <= 0) {
    p_delete(&screens_reply);
    return false;
  }

  xcb_xinerama_screen_info_t *screen_info =
    xcb_xinerama_query_screens_screen_info(screens_reply);
  if (!screen_info) {
    p_delete(&screens_reply);
    return false;
  }

  output_info_t *output_list = p_new(output_info_t, len);
  for (int i = 0; i < len; i++) {
    output_info_t *output = &output_list[i];
    output->geometry.x = screen_info[i].x_org;
    output->geometry.y = screen_info[i].y_org;
    output->geometry.width = screen_info[i].width;
    output->geometry.height = screen_info[i].height;
  }
  p_delete(&screens_reply);

  if (count) *count = (size_t)len;
  if (outputs) {
    *outputs = output_list;
  } else {
    for (size_t i = 0; i < (size_t)len; i++) p_delete(&output_list[i].name);
    p_delete(&output_list);
  }

  return true;
}

backend_detect_t *backend_detect(backend_t *backend) {
  output_info_t *outputs = nullptr;
  size_t count = 0;

  if (detect_monitor_by_randr(backend, &outputs, &count)) {
    backend_detect_t *detect = output_remove_duplication(outputs, count);
    p_delete(&outputs);
    return detect;
  }

  if (detect_monitor_by_xinerama(backend, &outputs, &count)) {
    backend_detect_t *detect = output_remove_duplication(outputs, count);
    p_delete(&outputs);
    return detect;
  }

  output_info_t *output = p_new(output_info_t, 1);
  output->geometry.x = 0;
  output->geometry.y = 0;
  output->geometry.width = backend->screen->width_in_pixels;
  output->geometry.height = backend->screen->height_in_pixels;

  backend_detect_t *detect = p_new(backend_detect_t, 1);
  detect->outputs = output;
  detect->output_count = 1;
  return detect;
}

void backend_detect_destroy(backend_detect_t *detect) {
  if (!detect) return;

  for (size_t i = 0; i < detect->output_count; i++) {
    p_delete(&detect->outputs[i].name);
  }

  p_delete(&detect->outputs);
  free(detect);
}
