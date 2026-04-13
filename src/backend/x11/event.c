#include "core/event.h"

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "backend/x11/window.h"
#include "base/memory.h"
#include "core/backend.h"
#include "core/types.h"
#include "core/window.h"
#include "internal.h"

static window_state_t *derive_window_states(
  const atoms_t *atoms,
  const xcb_atom_t *raw,
  uint32_t count,
  size_t *out_count
) {
  if (!raw || count == 0) {
    *out_count = 0;
    return nullptr;
  }
  window_state_t *buf = p_new(window_state_t, count);
  size_t valid        = 0;
  for (uint32_t i = 0; i < count; i++) {
    window_state_t s = atom_to_window_state(atoms, raw[i]);
    if (s != (window_state_t)-1) buf[valid++] = s;
  }
  if (valid == 0) {
    p_delete(&buf);
    *out_count = 0;
    return nullptr;
  }
  *out_count = valid;
  return buf;
}

static bool derive_skip_taskbar(
  const atoms_t *atoms,
  const xcb_atom_t *raw,
  uint32_t count
) {
  for (uint32_t i = 0; i < count; i++)
    if (raw[i] == atoms->_NET_WM_STATE_SKIP_TASKBAR) return true;
  return false;
}

static window_geometry_mode_t geometry_mode_from_hints(
  const xcb_icccm_wm_hints_t *hints
) {
  if (hints->initial_state == XCB_ICCCM_WM_STATE_ICONIC)
    return ZDWM_GEOMETRY_MINIMIZED;
  return ZDWM_GEOMETRY_NORMAL;
}

static window_geometry_mode_t geometry_mode_from_states(
  const atoms_t *atoms,
  const xcb_atom_t *state_atoms,
  uint32_t state_count
) {
  for (uint32_t i = 0; i < state_count; i++) {
    if (state_atoms[i] == atoms->_NET_WM_STATE_FULLSCREEN)
      return ZDWM_GEOMETRY_FULLSCREEN;
    if (state_atoms[i] == atoms->_NET_WM_STATE_MAXIMIZED_VERT ||
        state_atoms[i] == atoms->_NET_WM_STATE_MAXIMIZED_HORZ)
      return ZDWM_GEOMETRY_MAXIMIZED;
  }
  return ZDWM_GEOMETRY_NORMAL;
}

static bool handle_map_request(
  backend_t *backend,
  event_t *event,
  const xcb_map_request_event_t *xcb_event
) {
  xcb_window_t window = xcb_event->window;

  event_reset(event);
  event->type = ZDWM_EVENT_WINDOW_MAP_REQUEST;

  window_map_request_event_t *ev = &event->as.window_map_request;
  ev->window                     = (window_id_t)window;
  ev->transient_for              = window_get_transient_for(backend, window);
  ev->geometry_mode              = ZDWM_GEOMETRY_NORMAL;

  xcb_get_window_attributes_reply_t *wa =
    window_get_attributes(backend, window);
  if (!wa) return false;
  ev->override_redirect = (bool)wa->override_redirect;
  p_delete(&wa);

  xcb_icccm_wm_hints_t wm_hints = {0};
  if (window_get_wm_hints(backend, window, &wm_hints)) {
    ev->urgent        = (bool)xcb_icccm_wm_hints_get_urgency(&wm_hints);
    ev->geometry_mode = geometry_mode_from_hints(&wm_hints);
  }

  if (!window_get_fixed_size(backend, window, &ev->fixed_size)) return false;
  if (!window_get_geometry(backend, window, &ev->rect)) return false;

  const atoms_t *atoms    = &backend->atoms;
  xcb_atom_t *state_atoms = nullptr;
  uint32_t state_count    = 0;
  if (!window_get_atom_array(
        backend,
        window,
        atoms->_NET_WM_STATE,
        &state_atoms,
        &state_count
      )) {
    return false;
  }

  if (ev->geometry_mode == ZDWM_GEOMETRY_NORMAL && state_atoms) {
    ev->geometry_mode =
      geometry_mode_from_states(atoms, state_atoms, state_count);
  }

  if (state_atoms) {
    ev->skip_taskbar = derive_skip_taskbar(atoms, state_atoms, state_count);
    ev->props.states = derive_window_states(
      atoms,
      state_atoms,
      state_count,
      &ev->props.state_count
    );
  }

  p_delete(&state_atoms);

  window_layer_props_t *props = &ev->props;
  if (!window_get_types(backend, window, &props->types, &props->type_count)) {
    p_delete(&props->states);
    return false;
  }

  ev->metadata.role  = window_get_role(backend, window);
  ev->metadata.title = window_get_title(backend, window);
  window_get_class(
    backend,
    window,
    &ev->metadata.class_name,
    &ev->metadata.instance_name
  );

  return true;
}

static bool handle_unmap_notify(
  backend_t *backend,
  event_t *event,
  const xcb_unmap_notify_event_t *xcb_event
) {
  event_reset(event);
  event->type = ZDWM_EVENT_WINDOW_REMOVE;
  /* TODO: */
  return true;
}

static bool handle_destroy_notify(
  backend_t *backend,
  event_t *event,
  const xcb_destroy_notify_event_t *xcb_event
) {
  event_reset(event);
  event->type = ZDWM_EVENT_WINDOW_REMOVE;
  /* TODO: */
  return true;
}

static bool handle_configure_request(
  backend_t *backend,
  event_t *event,
  const xcb_configure_request_event_t *xcb_event
) {
  event_reset(event);
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
    bool handled          = false;

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
    event_reset(event);
  }
}
