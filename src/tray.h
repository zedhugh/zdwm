#pragma once

#include <stdbool.h>
#include <xcb/xproto.h>

#include "types.h"

void tray_init(void);
void tray_cleanup(void);

bool tray_handle_client_message(xcb_client_message_event_t *ev);
bool tray_handle_configure_request(xcb_configure_request_event_t *ev);
bool tray_handle_map_request(xcb_map_request_event_t *ev);
bool tray_handle_destroy_notify(xcb_destroy_notify_event_t *ev);
bool tray_handle_unmap_notify(xcb_unmap_notify_event_t *ev);
bool tray_handle_property_notify(xcb_property_notify_event_t *ev);
bool tray_handle_selection_clear(xcb_selection_clear_event_t *ev);

uint16_t tray_get_width(monitor_t *monitor);
void tray_place(monitor_t *monitor, int16_t right_edge);
bool tray_is_window(xcb_window_t window);
