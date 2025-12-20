#pragma once

#include <stdint.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_xrm.h>
#include <xkbcommon/xkbcommon.h>

#include "types.h"

typedef struct wm_t {
  padding_t padding;
  uint16_t border_width;
  uint16_t bar_height;
  uint16_t layout_count;

  bool need_restart;
  GMainLoop *loop;

  client_t *client_list;
  client_t *client_stack_list;
  client_t *client_focused;
  monitor_t *monitor_list;
  monitor_t *current_monitor;
  const layout_t *layout_list;

  char *font_family;
  uint32_t font_size;
  uint32_t dpi;

  xcb_connection_t *xcb_conn;
  xcb_screen_t *screen;
  xcb_xrm_database_t *xrm;
  xcb_key_symbols_t *key_symbols;
  xcb_window_t wm_check_window;
  int default_screen;
  bool have_xfixes;
  bool have_xinerama;
  bool have_randr;
  bool have_xkb;

  uint8_t event_base_xkb;
  bool xkb_reload_keymap;
  bool xkb_update_pending;
  struct xkb_context *xkb_ctx;
  struct xkb_state *xkb_state;

  color_set_t color_set;
} wm_t;

extern wm_t wm;

void wm_restart(void);
void wm_quit(void);
