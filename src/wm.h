#pragma once

#include "types.h"

typedef struct wm_t {
  bool need_restart;
  GMainLoop *loop;

  client_t *client_list;
  client_t *client_stack_list;
  client_t *client_focused;
  monitor_t *monitor_list;
  monitor_t *current_monitor;

  xcb_connection_t *xcb_conn;
  xcb_screen_t *screen;
  int default_screen;
  bool have_xfixes;
  bool have_xinerama;
  bool have_randr;
} wm_t;

extern wm_t wm;

void wm_restart(void);
void wm_quit(void);
