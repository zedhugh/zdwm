#pragma once

#include <xcb/xcb.h>

#include "core/backend.h" /* IWYU pragma: keep */

struct backend_t {
  xcb_connection_t *conn;
  xcb_screen_t *screen;
  int screenp;
  bool have_xfixes;
};
