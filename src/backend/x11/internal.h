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
  xcb_atom_t _NET_WM_STATE;
  xcb_atom_t _NET_WM_STATE_FULLSCREEN;
  xcb_atom_t _NET_WM_STATE_ABOVE;
  xcb_atom_t _NET_WM_STATE_STICKY;
  xcb_atom_t _NET_WM_STATE_MODAL;
  xcb_atom_t _NET_WM_STATE_SKIP_TASKBAR;

  xcb_atom_t _NET_WM_WINDOW_TYPE;
  xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DESKTOP;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
  xcb_atom_t _NET_WM_WINDOW_TYPE_TOOLBAR;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DIALOG;
  xcb_atom_t _NET_WM_WINDOW_TYPE_UTILITY;
  xcb_atom_t _NET_WM_WINDOW_TYPE_SPLASH;
  xcb_atom_t _NET_WM_WINDOW_TYPE_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_POPUP_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_TOOLTIP;
  xcb_atom_t _NET_WM_WINDOW_TYPE_COMBO;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DND;
  xcb_atom_t _NET_WM_WINDOW_TYPE_NOTIFICATION;
} atoms_t;

struct backend_t {
  xcb_connection_t *conn;
  xcb_screen_t *screen;
  int screenp;
  bool have_xfixes;

  atoms_t atoms;
};
