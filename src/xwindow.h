#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "action.h"
#include "xcursor.h"

typedef struct visual_t {
  uint8_t depth;
  xcb_visualtype_t *visual;
} visual_t;

visual_t *xwindow_get_xcb_visual(bool prefer_alpha);
void xwindow_change_cursor(xcb_window_t window, cursor_t cursor);
void xwindow_grab_keys(xcb_window_t window, const keyboard_t *keys,
                       int keys_length);
bool xwindow_send_event(xcb_window_t window, xcb_atom_t atom);
void xwindow_focus(xcb_window_t window);
