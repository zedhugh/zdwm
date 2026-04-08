#include "core/event.h"

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

#include "backend/x11/window.h"
#include "base/memory.h"
#include "core/backend.h"
#include "core/window.h"
#include "internal.h" /* IWYU pragma: keep */

static bool handle_map_request(backend_t *backend, event_t *event,
                               const xcb_map_request_event_t *xcb_event) {
  p_clear(event, 1);
  xcb_window_t window = xcb_event->window;
  event->type = ZDWM_EVENT_WINDOW_MAP_REQUEST;
  event->data.window_map_request.window = window;

  window_metadata_t *metadata = &event->data.window_map_request.metadata;
  metadata->role = window_get_role(backend, window);
  metadata->title = window_get_title(backend, window);
  window_get_class(backend, window, &metadata->class_name,
                   &metadata->instance_name);

  window_props_t *props = &event->data.window_map_request.props;
  props->transient_for = window_get_transient_for(backend, window);
  xcb_get_window_attributes_reply_t *wa_reply =
    window_get_attributes(backend, window);
  props->override_redirect = (bool)wa_reply->override_redirect;
  p_delete(&wa_reply);
  props->skip_taskbar = window_get_skip_taskbar(backend, window);
  props->urgent = window_get_urgency(backend, window);
  window_get_types(backend, window, &props->types, &props->type_count);
  window_get_states(backend, window, &props->states, &props->state_count);
  /* TODO: */

  return true;
}

static bool handle_unmap_notify(backend_t *backend, event_t *event,
                                const xcb_unmap_notify_event_t *xcb_event) {
  p_clear(event, 1);
  event->type = ZDWM_EVENT_WINDOW_REMOVE;
  /* TODO: */
  return true;
}

static bool handle_destroy_notify(backend_t *backend, event_t *event,
                                  const xcb_destroy_notify_event_t *xcb_event) {
  p_clear(event, 1);
  event->type = ZDWM_EVENT_WINDOW_REMOVE;
  /* TODO: */
  return true;
}

static bool handle_configure_request(
  backend_t *backend, event_t *event,
  const xcb_configure_request_event_t *xcb_event) {
  p_clear(event, 1);
  event->type = ZDWM_EVENT_CONFIGURE_REQUEST;
  /* TODO: */
  return true;
}

bool backend_next_event(backend_t *backend, event_t *event) {
  if (!backend || !backend->conn || !event) return false;

  for (;;) {
    xcb_generic_event_t *raw_event = xcb_wait_for_event(backend->conn);
    if (!raw_event) return false;

    uint8_t response_type = XCB_EVENT_RESPONSE_TYPE(raw_event);
    bool handled = false;

    printf("xcb event type: %u\n", response_type);

    switch (response_type) {
#define EVENT(type, handler)                              \
  case type:                                              \
    handled = handler(backend, event, (void *)raw_event); \
    break

      EVENT(XCB_MAP_REQUEST, handle_map_request);
      EVENT(XCB_UNMAP_NOTIFY, handle_unmap_notify);
      EVENT(XCB_DESTROY_NOTIFY, handle_destroy_notify);
      EVENT(XCB_CONFIGURE_REQUEST, handle_configure_request);

#undef EVENT

      default:
        break;
    }

    p_delete(&raw_event);
    if (handled) return true;
  }
}
