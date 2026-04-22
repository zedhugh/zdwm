#include "core/backend.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>

#include "backend/output_utils.h"
#include "backend/x11/window.h"
#include "base/array.h"
#include "base/log.h"
#include "base/macros.h"
#include "base/memory.h"
#include "core/plan.h"
#include "core/types.h"
#include "internal.h"

typedef struct atom_item_t {
  const char *name;
  size_t len;
  xcb_atom_t *atom;
} atom_item_t;

static void atoms_init(backend_t *backend) {
  xcb_connection_t *conn = backend->conn;
  atoms_t *atoms         = &backend->atoms;

#define ATOM_ITEM(name) {#name, sizeof(#name) - 1, &atoms->name},
  atom_item_t atom_list[] = {ATOM_LIST(ATOM_ITEM)};
#undef ATOM_ITEM

  xcb_intern_atom_cookie_t cookies[countof(atom_list)];
  for (size_t i = 0; i < countof(atom_list); ++i) {
    const atom_item_t *atom = &atom_list[i];
    cookies[i] = xcb_intern_atom_unchecked(conn, false, atom->len, atom->name);
  }

  xcb_intern_atom_reply_t *reply = nullptr;
  for (size_t i = 0; i < countof(atom_list); ++i) {
    reply = xcb_intern_atom_reply(conn, cookies[i], nullptr);
    if (!reply) continue;

    atom_item_t *atom = &atom_list[i];
    *atom->atom       = reply->atom;
    p_delete(&reply);
  }
}

static void create_wm_check_window(backend_t *backend) {
  xcb_connection_t *conn = backend->conn;
  xcb_window_t root      = backend->screen->root;
  uint8_t depth          = backend->screen->root_depth;
  xcb_visualid_t v       = backend->screen->root_visual;
  uint16_t _c            = XCB_WINDOW_CLASS_COPY_FROM_PARENT;
  uint32_t m             = XCB_NONE;
  void *p                = nullptr;

  xcb_window_t win = xcb_generate_id(conn);
  xcb_create_window(conn, depth, win, root, -1, -1, 1, 1, 0, _c, v, m, p);
  window_set_class_instance(conn, win);

#define NAME "zdwm"
  window_set_name_static(conn, win, NAME);
  uint8_t mode    = XCB_PROP_MODE_REPLACE;
  xcb_atom_t prop = backend->atoms._NET_WM_NAME;
  xcb_atom_t type = backend->atoms.UTF8_STRING;
  xcb_change_property(conn, mode, win, prop, type, 8, sizeof(NAME) - 1, NAME);
#undef NAME

  prop = backend->atoms._NET_SUPPORTING_WM_CHECK;
  type = XCB_ATOM_WINDOW;
  xcb_change_property(conn, mode, win, prop, type, 32, 1, &win);
  xcb_change_property(conn, mode, root, prop, type, 32, 1, &win);

  prop      = backend->atoms._NET_WM_PID;
  type      = XCB_ATOM_CARDINAL;
  pid_t pid = getpid();
  xcb_change_property(conn, mode, win, prop, type, 32, 1, &pid);
}

static void create_no_focus_window(backend_t *backend) {
  xcb_connection_t *conn = backend->conn;
  xcb_window_t root      = backend->screen->root;
  uint8_t d              = backend->screen->root_depth;
  xcb_visualid_t v       = backend->screen->root_visual;
  uint16_t _c            = XCB_WINDOW_CLASS_COPY_FROM_PARENT;

  xcb_window_t win = xcb_generate_id(conn);
  uint32_t m       = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
               XCB_CW_OVERRIDE_REDIRECT | XCB_CW_COLORMAP;
  const xcb_create_window_value_list_t p = {
    .override_redirect = true,
    .background_pixel  = backend->screen->black_pixel,
    .border_pixel      = backend->screen->black_pixel,
    .colormap          = backend->screen->default_colormap,
  };
  xcb_create_window_aux(conn, d, win, root, -1, -1, 1, 1, 0, _c, v, m, &p);
  window_set_class_instance(conn, win);
  window_set_name_static(conn, win, "zdwm no input window");
  xcb_map_window(conn, win);

  backend->window_no_focus = win;
}

backend_t *backend_create(const char *display_name) {
  int screen_num         = 0;
  xcb_connection_t *conn = xcb_connect(display_name, &screen_num);
  int xcb_conn_error     = xcb_connection_has_error(conn);
  if (xcb_conn_error) {
    fatal("cannot open display %s, error %d", display_name, xcb_conn_error);
  }

  xcb_screen_t *screen = xcb_aux_get_screen(conn, screen_num);
  if (!screen) fatal("cannot get screen info");
  xcb_window_t root = screen->root;

  uint32_t mask                = XCB_CW_EVENT_MASK;
  const xcb_params_cw_t params = {
    .event_mask = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
  };
  xcb_void_cookie_t cookie =
    xcb_aux_change_window_attributes_checked(conn, root, mask, &params);
  if (xcb_request_check(conn, cookie)) {
    fatal(
      "another window manager is already running (cannot select "
      "SubstructureRedirect)"
    );
  }

  backend_t *backend = p_new(backend_t, 1);
  backend->conn      = conn;
  backend->screen    = screen;
  backend->screenp   = screen_num;

  xcb_prefetch_extension_data(conn, &xcb_xfixes_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(conn, &xcb_xfixes_id);
  if (query && query->present) {
    xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(
      conn,
      XCB_XFIXES_MAJOR_VERSION,
      XCB_XFIXES_MINOR_VERSION
    );
    xcb_xfixes_query_version_reply_t *reply =
      xcb_xfixes_query_version_reply(conn, cookie, nullptr);

    if (reply) {
      backend->have_xfixes = true;
      p_delete(&reply);
    }
  } else {
    backend->have_xfixes = false;
  }

  atoms_init(backend);
  create_wm_check_window(backend);
  create_no_focus_window(backend);

  return backend;
}

void backend_destroy(backend_t *backend) {
  if (!backend) return;

  p_delete(&backend->config_list.cfgs);
  p_clear(&backend->config_list, 1);

  p_delete(&backend->unmap.windows);
  p_clear(&backend->unmap, 1);

  p_delete(&backend->kill.windows);
  p_clear(&backend->kill, 1);

  xcb_disconnect(backend->conn);
  backend->conn = nullptr;
  free(backend);
}

static bool detect_monitor_by_randr(
  const backend_t *backend,
  output_info_t **outputs,
  size_t *count
) {
  xcb_prefetch_extension_data(backend->conn, &xcb_randr_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(backend->conn, &xcb_randr_id);
  if (!query || !query->present) return false;

  xcb_randr_query_version_cookie_t cookie = xcb_randr_query_version(
    backend->conn,
    XCB_RANDR_MAJOR_VERSION,
    XCB_RANDR_MINOR_VERSION
  );
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
    output_info_t *output   = &output_list[i];
    output->geometry.x      = iter.data->x;
    output->geometry.y      = iter.data->y;
    output->geometry.width  = iter.data->width;
    output->geometry.height = iter.data->height;

    xcb_get_atom_name_cookie_t name_cookie =
      xcb_get_atom_name_unchecked(backend->conn, iter.data->name);
    xcb_get_atom_name_reply_t *name_reply =
      xcb_get_atom_name_reply(backend->conn, name_cookie, nullptr);

    if (name_reply) {
      char *name   = xcb_get_atom_name_name(name_reply);
      int length   = xcb_get_atom_name_name_length(name_reply);
      output->name = p_strndup(name, length);
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

static bool detect_monitor_by_xinerama(
  const backend_t *backend,
  output_info_t **outputs,
  size_t *count
) {
  xcb_prefetch_extension_data(backend->conn, &xcb_xinerama_id);
  const xcb_query_extension_reply_t *query =
    xcb_get_extension_data(backend->conn, &xcb_xinerama_id);
  if (!query || !query->present) return false;

  xcb_xinerama_query_version_cookie_t cookie = xcb_xinerama_query_version(
    backend->conn,
    XCB_XINERAMA_MAJOR_VERSION,
    XCB_XINERAMA_MINOR_VERSION
  );
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
    output_info_t *output   = &output_list[i];
    output->geometry.x      = screen_info[i].x_org;
    output->geometry.y      = screen_info[i].y_org;
    output->geometry.width  = screen_info[i].width;
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
  size_t count           = 0;

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

  output_info_t *output   = p_new(output_info_t, 1);
  output->geometry.x      = 0;
  output->geometry.y      = 0;
  output->geometry.width  = backend->screen->width_in_pixels;
  output->geometry.height = backend->screen->height_in_pixels;

  backend_detect_t *detect = p_new(backend_detect_t, 1);
  detect->outputs          = output;
  detect->output_count     = 1;
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

static void backend_focus_window(backend_t *backend, xcb_window_t window) {
  xcb_connection_t *conn = backend->conn;
  xcb_window_t root      = backend->screen->root;
  const atoms_t *atoms   = &backend->atoms;
  xcb_atom_t property    = atoms->_NET_ACTIVE_WINDOW;
  xcb_timestamp_t time   = XCB_TIME_CURRENT_TIME;
  if (window == XCB_WINDOW_NONE || window == root) {
    window = backend->window_no_focus;
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, window, time);
    xcb_delete_property(conn, root, property);
  } else {
    uint8_t mode    = XCB_PROP_MODE_REPLACE;
    xcb_atom_t type = XCB_ATOM_WINDOW;
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, window, time);
    xcb_change_property(conn, mode, root, property, type, 32, 1, &window);
    window_takefocus(backend, window);
  }
}

static inline void backend_change_window_list(
  backend_t *backend,
  bool stack_list,
  const effect_window_list_t *list
) {
  xcb_change_property(
    backend->conn,
    XCB_PROP_MODE_REPLACE,
    backend->screen->root,
    stack_list ? backend->atoms._NET_CLIENT_LIST_STACKING
               : backend->atoms._NET_CLIENT_LIST,
    XCB_ATOM_WINDOW,
    32,
    list->count,
    list->windows
  );
}

static void
backend_restack_windows(backend_t *backend, const effect_window_list_t *list) {
  if (list->count == 0) return;

  xcb_connection_t *conn      = backend->conn;
  xcb_window_t sibling_window = backend->window_no_focus;
  for (size_t i = 0; i < list->count; ++i) {
    uint16_t mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
    xcb_configure_window_value_list_t params = {
      .sibling    = sibling_window,
      .stack_mode = XCB_STACK_MODE_ABOVE
    };
    sibling_window = list->windows[i];
    xcb_configure_window_aux(conn, sibling_window, mask, &params);
  }
}

static void window_configure_list_reset(window_configure_list_t *configs) {
  p_clear(configs->cfgs, configs->count);
  configs->count = 0;
}

static void backend_apply_window_configure_list(backend_t *backend) {
  xcb_connection_t *conn           = backend->conn;
  window_configure_list_t *configs = &backend->config_list;
  for (size_t i = 0; i < configs->count; ++i) {
    window_configure_t *cfg = &configs->cfgs[i];
    if (!cfg->mask) continue;

    xcb_configure_window_aux(conn, cfg->window, cfg->mask, &cfg->value);
  }
}

static window_configure_t *
find_or_push_configure(window_configure_list_t *configs, xcb_window_t window) {
  window_configure_t *cfg = nullptr;
  for (size_t i = 0; i < configs->count; ++i) {
    cfg = &configs->cfgs[i];
    if (cfg->window == window) return cfg;
  }

  cfg = array_push(configs->cfgs, configs->count, configs->capacity);
  p_clear(cfg, 1);
  cfg->window = window;
  cfg->mask   = 0;
  return cfg;
}

static void backend_merge_effects(
  backend_t *backend,
  const effect_t *effects,
  size_t effect_count
) {
  window_configure_list_t *configs = &backend->config_list;

  for (size_t i = 0; i < effect_count; ++i) {
    const effect_t *e = &effects[i];
    switch (e->type) {
    case ZDWM_EFFECT_MAP_WINDOW:
      window_list_push(&backend->map, e->as.map.window);
      break;
    case ZDWM_EFFECT_UNMAP_WINDOW:
      window_list_push(&backend->unmap, e->as.unmap.window);
      break;
    case ZDWM_EFFECT_FOCUS_WINDOW:
      backend->focus_window = e->as.focus.window;
      backend->update_focus = true;
      break;
    case ZDWM_EFFECT_KILL_WINDOW:
      window_list_push(&backend->kill, e->as.kill.window);
      break;
    case ZDWM_EFFECT_MOVE_WINDOW: {
      window_configure_t *cfg =
        find_or_push_configure(configs, e->as.move.window);
      cfg->mask    |= XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
      cfg->value.x  = e->as.move.left_top_point.x;
      cfg->value.y  = e->as.move.left_top_point.y;
    } break;
    case ZDWM_EFFECT_RESIZE_WINDOW: {
      window_configure_t *cfg =
        find_or_push_configure(configs, e->as.resize.window);
      cfg->mask         |= XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
      cfg->value.width   = (uint32_t)e->as.resize.width;
      cfg->value.height  = (uint32_t)e->as.resize.height;
    } break;
    case ZDWM_EFFECT_CHANGE_BORDER_COLOR: {
      xcb_change_window_attributes_value_list_t value = {
        .border_pixel = e->as.change_border_color.color->argb
      };
      xcb_change_window_attributes_aux(
        backend->conn,
        e->as.change_border_color.window,
        XCB_CW_BORDER_PIXEL,
        &value
      );
    } break;
    case ZDWM_EFFECT_CHANGE_BORDER_WIDTH: {
      window_configure_t *cfg =
        find_or_push_configure(configs, e->as.change_border_width.window);
      cfg->mask               |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
      cfg->value.border_width  = e->as.change_border_width.border_width;
    } break;
    case ZDWM_EFFECT_CHANGE_WINDOW_LIST:
      backend_change_window_list(backend, false, &e->as.change_window_list);
      break;
    case ZDWM_EFFECT_RESTACK_WINDOWS:
      backend_restack_windows(backend, &e->as.restack_windows);
      backend_change_window_list(backend, true, &e->as.restack_windows);
      break;
    }
  }
}

static void backend_batch_apply_effects(backend_t *backend) {
  xcb_connection_t *conn = backend->conn;
  if (backend->unmap.count) {
    xcb_grab_server(conn);
    window_clean_event_mask(conn, backend->screen->root);

    window_list_t *unmap = &backend->unmap;
    for (size_t i = 0; i < unmap->count; ++i) {
      xcb_window_t window = unmap->windows[i];
      window_clean_event_mask(conn, window);
      xcb_unmap_window(conn, window);
    }

    root_set_event_mask(backend);
    xcb_ungrab_server(conn);
  }

  for (size_t i = 0; i < backend->map.count; ++i) {
    xcb_window_t window = backend->map.windows[i];
    window_set_event_mask(conn, window);
    xcb_map_window(conn, window);
  }

  for (size_t i = 0; i < backend->kill.count; ++i) {
    window_kill(backend, backend->kill.windows[i]);
  }

  backend_apply_window_configure_list(backend);

  if (backend->update_focus) {
    backend_focus_window(backend, backend->focus_window);
  }
}

bool backend_apply_effect(
  backend_t *backend,
  const effect_t *effects,
  size_t effect_count
) {
  backend->update_focus = false;
  window_configure_list_reset(&backend->config_list);
  window_list_reset(&backend->unmap);
  window_list_reset(&backend->map);
  window_list_reset(&backend->kill);

  backend_merge_effects(backend, effects, effect_count);
  backend_batch_apply_effects(backend);

  xcb_flush(backend->conn);
  return true;
}
