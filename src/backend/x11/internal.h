#pragma once

#include <stddef.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "base/window_list.h"

#define ATOM_LIST(X)                   \
  X(COMPOUND_TEXT)                     \
  X(UTF8_STRING)                       \
                                       \
  X(WM_WINDOW_ROLE)                    \
  X(WM_NAME)                           \
  X(_NET_WM_NAME)                      \
                                       \
  X(WM_PROTOCOLS)                      \
  X(WM_TAKE_FOCUS)                     \
  X(WM_DELETE_WINDOW)                  \
  X(_NET_ACTIVE_WINDOW)                \
                                       \
  X(_NET_SUPPORTING_WM_CHECK)          \
  X(_NET_WM_PID)                       \
                                       \
  X(_NET_CLIENT_LIST)                  \
  X(_NET_CLIENT_LIST_STACKING)         \
                                       \
  X(_NET_WM_STATE)                     \
  X(_NET_WM_STATE_FULLSCREEN)          \
  X(_NET_WM_STATE_ABOVE)               \
  X(_NET_WM_STATE_STICKY)              \
  X(_NET_WM_STATE_MODAL)               \
  X(_NET_WM_STATE_SKIP_TASKBAR)        \
  X(_NET_WM_STATE_MAXIMIZED_VERT)      \
  X(_NET_WM_STATE_MAXIMIZED_HORZ)      \
                                       \
  X(_NET_WM_STATE_DEMANDS_ATTENTION)   \
                                       \
  X(_NET_WM_WINDOW_TYPE)               \
  X(_NET_WM_WINDOW_TYPE_NORMAL)        \
  X(_NET_WM_WINDOW_TYPE_DESKTOP)       \
  X(_NET_WM_WINDOW_TYPE_DOCK)          \
  X(_NET_WM_WINDOW_TYPE_TOOLBAR)       \
  X(_NET_WM_WINDOW_TYPE_DIALOG)        \
  X(_NET_WM_WINDOW_TYPE_UTILITY)       \
  X(_NET_WM_WINDOW_TYPE_SPLASH)        \
  X(_NET_WM_WINDOW_TYPE_MENU)          \
  X(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU) \
  X(_NET_WM_WINDOW_TYPE_POPUP_MENU)    \
  X(_NET_WM_WINDOW_TYPE_TOOLTIP)       \
  X(_NET_WM_WINDOW_TYPE_COMBO)         \
  X(_NET_WM_WINDOW_TYPE_DND)           \
  X(_NET_WM_WINDOW_TYPE_NOTIFICATION)

typedef struct atoms_t {
#define DECLARATION_ATOM(name) xcb_atom_t name;
  ATOM_LIST(DECLARATION_ATOM);
#undef DECLARATION_ATOM
} atoms_t;

typedef struct window_configure_t {
  xcb_window_t window;
  uint32_t mask;
  xcb_configure_window_value_list_t value;
} window_configure_t;

typedef struct window_configure_list_t {
  window_configure_t *cfgs;
  size_t count;
  size_t capacity;
} window_configure_list_t;

struct backend_t {
  xcb_key_symbols_t *key_symbols;
  xcb_connection_t *conn;
  xcb_screen_t *screen;
  int screenp;
  bool have_xfixes;

  atoms_t atoms;

  /* When no window should take focus, then focus this window */
  xcb_window_t window_no_focus;
  xcb_window_t wm_check_window;

  xcb_window_t focus_window;
  bool update_focus;

  window_configure_list_t config_list;
  window_list_t unmap;
  window_list_t map;
  window_list_t kill;
};
