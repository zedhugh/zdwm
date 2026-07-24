#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "interface/tray.h"

extern const tray_api_t tray_api;

typedef struct backend_t backend_t;

void tray_init(
  backend_t *backend,
  xcb_window_t host_window,
  int32_t host_width,
  int32_t host_height
);
void tray_cleanup(backend_t *backend);
