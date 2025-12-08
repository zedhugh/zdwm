#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "action.h"
#include "app.h"
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
void xwindow_get_text_property(xcb_window_t window, xcb_atom_t property,
                               char **out);

#define xwindow_set_name_static(window, name)                    \
  xcb_icccm_set_wm_name(wm.xcb_conn, window, XCB_ATOM_STRING, 8, \
                        sizeof(name) - 1, name)
#define xwindow_set_class_instance(window) \
  xwindow_set_class_instance_static(window, APP_NAME, APP_NAME)
#define xwindow_set_class_instance_static(window, class, instance) \
  _xwindow_set_class_instance_static(window, instance "\0" class)
#define _xwindow_set_class_instance_static(window, instance_class)    \
  xcb_icccm_set_wm_class(wm.xcb_conn, window, sizeof(instance_class), \
                         instance_class)
