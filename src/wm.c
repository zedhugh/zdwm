#include "wm.h"

#include <glib-unix.h>
#include <glib.h>
#include <glibconfig.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>
#include <xcb/xproto.h>

#include "app.h"
#include "atoms-extern.h"
#include "atoms.h"
#include "backtrace.h"
#include "base.h"
#include "buffer.h"
#include "client.h"
#include "color.h"
#include "default_config.h"
#include "event.h"
#include "monitor.h"
#include "text.h"
#include "types.h"
#include "utils.h"
#include "xcursor.h"
#include "xkb.h"
#include "xres.h"
#include "xwindow.h"

static void wm_setup_signal(void);
static void wm_check_other_wm(void);
static void wm_check_xcb_extensions(void);
static void wm_detect_monitor(void);
static void wm_scan_clients(void);
static void wm_clean(void);
static void wm_setup(void);
static void wm_get_xres_config(void);
static void wm_init_color_set(void);
static void wm_setup_keybindings(void);
static void wm_run_autostart(const char *const commands[]);
static void run_once(const char *command);
static bool command_already_running(const char *command);

wm_t wm;

/** argv used to run wm */
static char **cmd_argv;

static gboolean restart_on_signal(gpointer data) {
  wm_restart();
  return TRUE;
}

static gboolean exit_on_signal(gpointer data) {
  g_main_loop_quit(wm.loop);
  return TRUE;
}

static void signal_fatal(int signal_number) {
  buffer_t buffer;
  backtrace_get(&buffer);
  fatal("signal %d, dumping backtrace\n%s", signal_number, buffer.s);
}

static guint sources[3] = {0};

void wm_setup_signal(void) {
  sources[0] = g_unix_signal_add(SIGINT, exit_on_signal, nullptr);
  sources[1] = g_unix_signal_add(SIGTERM, exit_on_signal, nullptr);
  sources[2] = g_unix_signal_add(SIGHUP, restart_on_signal, nullptr);

  struct sigaction sa = {.sa_handler = signal_fatal, .sa_flags = SA_RESETHAND};
  sigemptyset(&sa.sa_mask);
  sigaction(SIGABRT, &sa, 0);
  sigaction(SIGBUS, &sa, 0);
  sigaction(SIGFPE, &sa, 0);
  sigaction(SIGILL, &sa, 0);
  sigaction(SIGSEGV, &sa, 0);
  signal(SIGPIPE, SIG_IGN);
}

void wm_check_other_wm(void) {
  /* connect X server */
  int default_screen;
  xcb_connection_t *conn = xcb_connect(nullptr, &default_screen);
  int xcb_conn_error = xcb_connection_has_error(conn);
  if (xcb_conn_error) fatal("cannot open display (error %d)", xcb_conn_error);

  xcb_screen_t *screen = xcb_aux_get_screen(conn, default_screen);
  xcb_window_t root = screen->root;

  /* check other window manager running */
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

  wm.xcb_conn = conn;
  wm.default_screen = default_screen;
  wm.screen = screen;
}

void wm_check_xcb_extensions(void) {
  xcb_prefetch_extension_data(wm.xcb_conn, &xcb_xfixes_id);
  xcb_prefetch_extension_data(wm.xcb_conn, &xcb_xinerama_id);
  xcb_prefetch_extension_data(wm.xcb_conn, &xcb_randr_id);

  const xcb_query_extension_reply_t *query;
  query = xcb_get_extension_data(wm.xcb_conn, &xcb_xfixes_id);
  wm.have_xfixes = query && query->present;

  query = xcb_get_extension_data(wm.xcb_conn, &xcb_xinerama_id);
  wm.have_xinerama = query && query->present;

  query = xcb_get_extension_data(wm.xcb_conn, &xcb_randr_id);
  wm.have_randr = query && query->present;
}

static bool wm_detect_monitor_by_randr(void) {
  xcb_randr_get_monitors_cookie_t monitors_cookie =
    xcb_randr_get_monitors(wm.xcb_conn, wm.screen->root, 1);
  xcb_randr_get_monitors_reply_t *monitors_reply =
    xcb_randr_get_monitors_reply(wm.xcb_conn, monitors_cookie, nullptr);
  if (monitors_reply == nullptr) {
    warn("RandR get monitor failed");
    return false;
  }

  monitor_t *prev_monitor = nullptr;
  xcb_randr_monitor_info_iterator_t monitor_iter =
    xcb_randr_get_monitors_monitors_iterator(monitors_reply);
  for (; monitor_iter.rem; xcb_randr_monitor_info_next(&monitor_iter)) {
    monitor_t *monitor = p_new(monitor_t, 1);
    monitor->geometry.x = monitor_iter.data->x;
    monitor->geometry.y = monitor_iter.data->y;
    monitor->geometry.width = monitor_iter.data->width;
    monitor->geometry.height = monitor_iter.data->height;

    xcb_get_atom_name_cookie_t name_cookie =
      xcb_get_atom_name_unchecked(wm.xcb_conn, monitor_iter.data->name);
    xcb_get_atom_name_reply_t *name_reply =
      xcb_get_atom_name_reply(wm.xcb_conn, name_cookie, nullptr);

    if (name_reply) {
      char *name = xcb_get_atom_name_name(name_reply);
      int length = xcb_get_atom_name_name_length(name_reply);
      monitor->name = memcpy(p_new(char, length + 1), name, length);
      monitor->name[length] = '\0';
      p_delete(&name_reply);
    } else {
      monitor->name = strdup("unknown");
    }

    if (prev_monitor) {
      prev_monitor->next = monitor;
    } else {
      wm.monitor_list = monitor;
      wm.current_monitor = monitor;
    }

    prev_monitor = monitor;
  }

  p_delete(&monitors_reply);

  return true;
}

static bool wm_detect_monitor_by_xinerama(void) {
  xcb_xinerama_is_active_cookie_t active_cookie =
    xcb_xinerama_is_active(wm.xcb_conn);
  xcb_xinerama_is_active_reply_t *active_reply =
    xcb_xinerama_is_active_reply(wm.xcb_conn, active_cookie, nullptr);
  if (!active_reply || !active_reply->state) {
    p_delete(&active_reply);
    return false;
  }

  xcb_xinerama_query_screens_cookie_t screens_cookie =
    xcb_xinerama_query_screens(wm.xcb_conn);
  xcb_xinerama_query_screens_reply_t *screens_reply =
    xcb_xinerama_query_screens_reply(wm.xcb_conn, screens_cookie, nullptr);

  if (screens_reply == nullptr) return false;

  int count = xcb_xinerama_query_screens_screen_info_length(screens_reply);
  if (count <= 0) {
    p_delete(&screens_reply);
    return false;
  }

  xcb_xinerama_screen_info_t *screen_info =
    xcb_xinerama_query_screens_screen_info(screens_reply);

  monitor_t *prev_monitor = nullptr;
  for (int i = 0; i < count; i++) {
    monitor_t *monitor = p_new(monitor_t, 1);
    monitor->geometry.x = screen_info[i].x_org;
    monitor->geometry.y = screen_info[i].y_org;
    monitor->geometry.width = screen_info[i].width;
    monitor->geometry.height = screen_info[i].height;

    if (prev_monitor) {
      prev_monitor->next = monitor;
    } else {
      wm.monitor_list = monitor;
      wm.current_monitor = monitor;
    }

    prev_monitor = monitor;
  }
  p_delete(&screens_reply);

  return true;
}

void wm_detect_monitor(void) {
  if (wm.have_randr && wm_detect_monitor_by_randr()) return;
  if (wm.have_xinerama && wm_detect_monitor_by_xinerama()) return;

  if (wm.screen == nullptr) fatal("cannot detect monitor info");

  monitor_t *monitor = p_new(monitor_t, 1);
  monitor->geometry.x = 0;
  monitor->geometry.y = 0;
  monitor->geometry.width = wm.screen->width_in_pixels;
  monitor->geometry.height = wm.screen->height_in_pixels;

  wm.monitor_list = monitor;
  wm.current_monitor = monitor;
}

typedef struct window_list_t {
  xcb_window_t *normal_list;
  xcb_window_t *transient_list;
  uint32_t normal_count;
  uint32_t transient_count;
} window_list_t;
void wm_scan_clients(void) {
  xcb_query_tree_cookie_t cookie = xcb_query_tree(wm.xcb_conn, wm.screen->root);
  xcb_query_tree_reply_t *tree_reply =
    xcb_query_tree_reply(wm.xcb_conn, cookie, nullptr);
  if (!tree_reply) return;

  xcb_window_t *list = xcb_query_tree_children(tree_reply);
  int len = xcb_query_tree_children_length(tree_reply);
  if (!len) {
    p_delete(&tree_reply);
    return;
  }

  xcb_get_window_attributes_reply_t *wa_reply = nullptr;
  xcb_get_geometry_reply_t *geo_reply = nullptr;
  for (int i = 0; i < len; i++) {
    xcb_window_t window = list[i];
    wa_reply = xwindow_get_attributes_reply(window);
    if (!wa_reply) continue;

    uint8_t override_redirect = wa_reply->override_redirect;
    uint8_t map_state = wa_reply->map_state;
    p_delete(&wa_reply);

    if (override_redirect || xwindow_get_transient_for(window)) continue;

    geo_reply = xwindow_get_geometry_reply(window);
    if (!geo_reply) continue;

    if (map_state == XCB_MAP_STATE_VIEWABLE ||
        xwindow_get_state(window) == XCB_ICCCM_WM_STATE_ICONIC) {
      client_manage(window, geo_reply);
    }
    p_delete(&geo_reply);
  }
  for (int i = 0; i < len; i++) {
    xcb_window_t window = list[i];
    wa_reply = xwindow_get_attributes_reply(window);
    if (!wa_reply) continue;

    uint8_t map_state = wa_reply->map_state;
    p_delete(&wa_reply);

    geo_reply = xwindow_get_geometry_reply(window);
    if (!geo_reply) continue;

    if (xwindow_get_transient_for(window) &&
        (map_state == XCB_MAP_STATE_VIEWABLE ||
         xwindow_get_state(window) == XCB_ICCCM_WM_STATE_ICONIC)) {
      client_manage(window, geo_reply);
    }
    p_delete(&geo_reply);
  }
  p_delete(&tree_reply);
}

void wm_clean(void) {
  text_clean_pango_layout();
  monitor_clean(wm.monitor_list);

  for (int i = 0; i < countof(sources); i++) {
    guint source_id = sources[i];
    if (source_id) g_source_remove(source_id);
  }

  xcb_delete_property(wm.xcb_conn, wm.screen->root, _NET_CLIENT_LIST);
  xcb_delete_property(wm.xcb_conn, wm.screen->root, _NET_ACTIVE_WINDOW);
  xcb_delete_property(wm.xcb_conn, wm.screen->root, _NET_SUPPORTING_WM_CHECK);
  xcb_aux_sync(wm.xcb_conn);

  xcb_destroy_window(wm.xcb_conn, wm.wm_check_window);
  wm.wm_check_window = XCB_WINDOW_NONE;

  xcursor_clean();
  xkb_free();
  xcb_ungrab_key(wm.xcb_conn, XCB_GRAB_ANY, wm.screen->root, modifier_any);
  xcb_key_symbols_free(wm.key_symbols);
  xcb_aux_sync(wm.xcb_conn);
  xcb_disconnect(wm.xcb_conn);
}

static void wm_setup(void) {
  wm.layout_list = layout_list;
  wm.layout_count = (uint16_t)countof(layout_list);

  wm.rules = rules;
  wm.rules_count = countof(rules);

  wm_get_xres_config();
  wm_init_color_set();

  {
    int font_height =
      text_init_pango_layout(wm.font_family, wm.font_size, wm.dpi);
    wm.bar_height = (uint16_t)font_height + 2 * wm.padding.bar_y;
    logger("font height: %d, bar height: %u\n", font_height, wm.bar_height);
  }

  uint32_t tag_count = 0;
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    tag_count += monitor_initialize_tag(m, (const char **)tags, tag_count);
    monitor_init_bar(m);
    monitor_draw_bar(m);
  }

  uint32_t mask = XCB_CW_EVENT_MASK | XCB_CW_CURSOR | XCB_CW_BACK_PIXEL;
  xcb_params_cw_t params = {
    .back_pixel = wm.screen->black_pixel,
    .cursor = xcursor_get_xcb_cursor(cursor_normal),
    .event_mask =
      XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_KEY_PRESS,
  };
  xcb_aux_change_window_attributes(wm.xcb_conn, wm.screen->root, mask, &params);

  wm_setup_keybindings();
  atoms_init(wm.xcb_conn);

  {
    wm.wm_check_window = xcb_generate_id(wm.xcb_conn);
    xcb_create_window(wm.xcb_conn, wm.screen->root_depth, wm.wm_check_window,
                      wm.screen->root, -1, -1, 1, 1, 0, XCB_COPY_FROM_PARENT,
                      wm.screen->root_visual, XCB_NONE, nullptr);
    xwindow_set_class_instance(wm.wm_check_window);
#define NAME APP_NAME "_wm_check"
    xwindow_set_name_static(wm.wm_check_window, NAME);
    const void *data = &wm.wm_check_window;
    xcb_atom_t type = XCB_ATOM_WINDOW;
    uint8_t mode = XCB_PROP_MODE_REPLACE;
    xcb_change_property(wm.xcb_conn, mode, wm.wm_check_window, _NET_WM_NAME,
                        UTF8_STRING, 8, sizeof(NAME) - 1, NAME);
    xcb_change_property(wm.xcb_conn, mode, wm.wm_check_window,
                        _NET_SUPPORTING_WM_CHECK, type, 32, 1, data);
    xcb_change_property(wm.xcb_conn, mode, wm.screen->root,
                        _NET_SUPPORTING_WM_CHECK, type, 32, 1, data);
#undef NAME
  }

  xkb_init();
  xcb_flush(wm.xcb_conn);
}

static void wm_get_xres_config(void) {
  xres_init_xrm_db();

  wm.dpi = default_dpi;
  wm.font_size = font_size;
  xres_get_uint32("Xft.dpi", (uint32_t *)&wm.dpi);
  xres_get_uint32(APP_NAME ".font_size", (uint32_t *)&wm.font_size);
  xres_get_string(APP_NAME ".font_family", &wm.font_family, font_family);

  wm.border_width = border_width;
  xres_get_uint32(APP_NAME ".border_width", (uint32_t *)&wm.border_width);

  wm.padding.bar_y = bar_y_padding;
  wm.padding.tag_x = tag_x_padding;
#define NAME APP_NAME ".padding."
  xres_get_uint32(NAME "bar_y", (uint32_t *)&wm.padding.bar_y);
  xres_get_uint32(NAME "tag_x", (uint32_t *)&wm.padding.tag_x);
#undef NAME

  xres_clean();
}

void wm_init_color_set(void) {
  color_parse(bar_bg, &wm.color_set.bar_bg);
  color_parse(tag_bg, &wm.color_set.tag_bg);
  color_parse(active_tag_bg, &wm.color_set.active_tag_bg);
  color_parse(tag_color, &wm.color_set.tag_color);
  color_parse(active_tag_color, &wm.color_set.active_tag_color);
  color_parse(border_color, &wm.color_set.border_color);
  color_parse(active_border_color, &wm.color_set.active_border_color);
}

void wm_setup_keybindings(void) {
  if (wm.key_symbols) {
    xcb_key_symbols_free(wm.key_symbols);
    p_delete(&wm.key_symbols);
  }

  wm.key_symbols = xcb_key_symbols_alloc(wm.xcb_conn);
  xwindow_grab_keys(wm.screen->root, key_list, countof(key_list));
}

/**
 * @brief 允许自启动程序
 * @param commands 命令字符串数组(以 nullptr 结尾)
 */
void wm_run_autostart(const char *const commands[]) {
  for (int i = 0; commands[i]; i++) run_once(commands[i]);
}

void run_once(const char *command) {
  if (command_already_running(command)) return;
  g_spawn_command_line_async(command, nullptr);
}

bool command_already_running(const char *command) {
  char shell_cmd[1024];
  const char *user = getenv("USER");
  snprintf(shell_cmd, sizeof(shell_cmd), "pgrep -u %s -fx '%s'", user, command);

  char *output = nullptr;
  char *error = nullptr;
  g_spawn_command_line_sync(shell_cmd, &output, &error, nullptr, nullptr);

  bool result =
    (error == nullptr || !strlen(error)) && (output && strlen(output));
  p_delete(&output);
  p_delete(&error);

  return result;
}

void wm_restart(void) {
  wm.need_restart = true;
  if (g_main_loop_is_running(wm.loop)) {
    g_main_loop_quit(wm.loop);
  }
}

void wm_quit(void) {
  wm.need_restart = false;
  if (g_main_loop_is_running(wm.loop)) {
    g_main_loop_quit(wm.loop);
  }
}

void wm_restack_clients(void) {
  xcb_window_t sibling_window = XCB_WINDOW_NONE;
  for (client_t *c = wm.client_stack_list; c; c = c->stack_next) {
    uint16_t mask = XCB_CONFIG_WINDOW_STACK_MODE;
    xcb_params_configure_window_t params = {};
    if (sibling_window == XCB_WINDOW_NONE) {
      mask = XCB_CONFIG_WINDOW_STACK_MODE;
      params.stack_mode = XCB_STACK_MODE_ABOVE;
    } else {
      mask = XCB_CONFIG_WINDOW_STACK_MODE | XCB_CONFIG_WINDOW_SIBLING;
      params.stack_mode = XCB_STACK_MODE_BELOW;
      params.sibling = sibling_window;
    }
    sibling_window = c->window;
    xcb_aux_configure_window(wm.xcb_conn, c->window, mask, &params);
  }
  xcb_flush(wm.xcb_conn);
}

static inline int intersect(area_t area, monitor_t *m) {
  return MAX(0, MIN(area.x + area.width, m->geometry.x + m->geometry.width) -
                  MAX(area.x, m->geometry.x)) *
         MAX(0, MIN(area.y + area.height, m->geometry.y + m->geometry.height) -
                  MAX(area.y, m->geometry.y));
}

monitor_t *wm_get_monitor_by_area(area_t area) {
  monitor_t *monitor = wm.current_monitor;
  int temp_area, max_area = 0;
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    if ((temp_area = intersect(area, m)) > max_area) {
      max_area = temp_area;
      monitor = m;
    }
  }

  return monitor;
}

monitor_t *wm_get_monitor_by_point(point_t point) {
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    if (m->geometry.x <= point.x &&
        point.x < m->geometry.x + m->geometry.width) {
      return m;
    }
  }
  return wm.monitor_list;
}

monitor_t *wm_get_monitor_by_window(xcb_window_t window) {
  if (window == wm.screen->root) {
    xcb_query_pointer_cookie_t cookie = xcb_query_pointer(wm.xcb_conn, window);
    xcb_query_pointer_reply_t *reply =
      xcb_query_pointer_reply(wm.xcb_conn, cookie, nullptr);
    if (reply) {
      area_t area = {reply->root_x, reply->root_y, 1, 1};
      p_delete(&reply);
      return wm_get_monitor_by_area(area);
    }
  }

  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    if (window == m->bar_window) return m;
  }

  client_t *c = client_get_by_window(window);
  if (c) return c->monitor;

  return wm.current_monitor;
}

monitor_t *wm_get_next_monitor(monitor_t *monitor) {
  monitor_t *next_monitor = monitor->next;
  if (next_monitor == nullptr) next_monitor = wm.monitor_list;
  return next_monitor;
}

static void debug_show_monitor_list(void) {
  logger("\n========================== monitors ==========================\n");
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    logger("x: %4d, y: %4d, width: %4u, height: %4u -> monitor[%s]\n",
           m->geometry.x, m->geometry.y, m->geometry.width, m->geometry.height,
           m->name);
    logger("x: %4d, y: %4d, width: %4u, height: %4u -> workarea[%s]: %u\n",
           m->workarea.x, m->workarea.y, m->workarea.width, m->workarea.height,
           m->name, wm.bar_height);
    logger("tag extend: [%d, %d]\n", m->tag_extent.start, m->tag_extent.end);
    for (const tag_t *tag = m->tag_list; tag; tag = tag->next) {
      logger("tag[%u]: %4u, \"%s\", [%d, %d]\n", tag->index, tag->mask,
             tag->name, tag->bar_extent.start, tag->bar_extent.end);
    }
  }
}

int main(int argc, char *argv[]) {
  p_clear(&wm, 1);
  cmd_argv = argv;
  wm_setup_signal();

  wm_check_other_wm();
  wm_check_xcb_extensions();

  wm_detect_monitor();
  wm_setup();
  setup_event_loop();
  wm_scan_clients();

  debug_show_monitor_list();

  wm_run_autostart(autostart_list);

  if (wm.loop == nullptr) {
    wm.loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(wm.loop);
  }
  g_main_loop_unref(wm.loop);
  wm.loop = nullptr;

  wm_clean();

  if (wm.need_restart) {
    execvp(cmd_argv[0], cmd_argv);
    fatal("execv() failed: %s", strerror(errno));
  }

  return EXIT_SUCCESS;
}
