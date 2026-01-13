#pragma once

#include <X11/cursorfont.h>
#include <xcb/xproto.h>

#include "base.h"

typedef enum cursor_t {
  cursor_normal = XC_left_ptr,
  cursor_resize = XC_bottom_right_corner,
  cursor_move = XC_fleur,
} cursor_t;

xcb_cursor_t xcursor_get_xcb_cursor(cursor_t cursor);
void xcursor_clean(void);
point_t xcursor_query_pointer_position(void);
void xcursor_set_pointer_position(point_t point);
