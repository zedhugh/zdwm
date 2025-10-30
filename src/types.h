#pragma once

#include <glib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

typedef struct global_state_t {
  bool need_restart;
  GMainLoop *loop;

  xcb_connection_t *xcb_conn;
  xcb_screen_t *screen;
  int default_screen;
} global_state_t;

extern global_state_t global_state;
