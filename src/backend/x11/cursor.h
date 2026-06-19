#pragma once

#include <xcb/xproto.h>

#include "internal.h"

void cursor_init(backend_t *backend);
void cursor_cleanup(backend_t *backend);
xcb_cursor_t cursor_get_xcb_cursor(backend_t *backend, cursor_t cursor);
