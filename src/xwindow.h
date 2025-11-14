#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

typedef struct visual_t {
  uint8_t depth;
  xcb_visualtype_t *visual;
} visual_t;

visual_t *xwindow_get_xcb_visual(bool prefer_alpha);
