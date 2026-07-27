#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "interface/tray.h"

extern const tray_api_t tray_api;

typedef struct backend_t backend_t;

/* TODO: 添加窗口背景色 */
void tray_init(
  backend_t *backend,
  xcb_window_t host_window,
  int32_t host_width,
  int32_t host_height
);
void tray_cleanup(backend_t *backend);

bool tray_handle_client_message(
  backend_t *backend,
  const xcb_client_message_event_t *ev
);
bool tray_handle_configure_request(
  backend_t *backend,
  const xcb_configure_request_event_t *ev
);
bool tray_handle_map_request(
  backend_t *backend,
  const xcb_map_request_event_t *ev
);
bool tray_handle_property_notify(
  backend_t *backend,
  const xcb_property_notify_event_t *ev
);
