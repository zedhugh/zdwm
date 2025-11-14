#pragma once

#include <X11/cursorfont.h>
#include <xcb/xproto.h>

typedef enum cursor_t {
  cursor_normal = XC_left_ptr,
  cursor_resize = XC_bottom_right_corner,
  cursor_move = XC_fleur,
} cursor_t;

xcb_cursor_t xcursor_get_xcb_cursor(cursor_t cursor);
void xcursor_clean(void);
