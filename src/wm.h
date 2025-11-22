#pragma once

#include <stdint.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "types.h"

typedef struct wm_t {
  padding_t padding;
  uint16_t bar_height;

  bool need_restart;
  GMainLoop *loop;

  client_t *client_list;
  client_t *client_stack_list;
  client_t *client_focused;
  monitor_t *monitor_list;
  monitor_t *current_monitor;

  xcb_connection_t *xcb_conn;
  xcb_screen_t *screen;
  xcb_key_symbols_t *key_symbols;
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
