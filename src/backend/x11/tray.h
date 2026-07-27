#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "interface/tray.h"

extern const tray_api_t tray_api;

typedef struct backend_t backend_t;

typedef struct tray_host_window_t {
  xcb_window_t window;
  int32_t width;
  int32_t height;
  uint32_t bg_pixel;
} tray_host_window_t;

void tray_init(backend_t *backend, tray_host_window_t host_window);
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
bool tray_handle_unmap_notify(
  backend_t *backend,
  const xcb_unmap_notify_event_t *ev
);
bool tray_handle_destroy_notify(
  backend_t *backend,
  const xcb_destroy_notify_event_t *ev
);
