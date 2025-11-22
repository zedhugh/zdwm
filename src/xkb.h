#pragma once

#include <xcb/xcb.h>

void xkb_init(void);
void xkb_free(void);
void xkb_handle_event(xcb_generic_event_t *event);
