#pragma once

#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "core/backend.h" /* IWYU pragma: keep */

typedef struct atoms_t {
  xcb_atom_t COMPOUND_TEXT;
  xcb_atom_t UTF8_STRING;
  xcb_atom_t WM_WINDOW_ROLE;
  xcb_atom_t WM_NAME;
  xcb_atom_t _NET_WM_NAME;
} atoms_t;

struct backend_t {
  xcb_connection_t *conn;
  xcb_screen_t *screen;
  int screenp;
  bool have_xfixes;

  atoms_t atoms;
};
