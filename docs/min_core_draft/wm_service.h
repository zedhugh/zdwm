#pragma once

#include <stdint.h>

#include "wm_state.h"

typedef enum wm_service_event_type_t {
  WM_SERVICE_EVENT_NONE,
  WM_SERVICE_EVENT_RUNTIME_STARTED,
  WM_SERVICE_EVENT_RUNTIME_STOPPING,
  WM_SERVICE_EVENT_WINDOW_METADATA_CHANGED,
  WM_SERVICE_EVENT_STATUS_TICK,
  WM_SERVICE_EVENT_RENDER_OUTPUT,
} wm_service_event_type_t;

typedef struct wm_service_event_t {
  wm_service_event_type_t type;
  union {
    struct {
      wm_window_id_t window_id;
      uint32_t changed_fields;
    } window_metadata_changed;

    struct {
      wm_output_id_t output_id;
    } render_output;
  } as;
} wm_service_event_t;

typedef struct wm_service_t wm_service_t;

typedef struct wm_service_api_t {
  bool (*init)(wm_service_t *service);
  void (*handle_event)(wm_service_t *service, const wm_service_event_t *event,
                       const wm_state_t *state);
  void (*shutdown)(wm_service_t *service);
} wm_service_api_t;

struct wm_service_t {
  const wm_service_api_t *api;
  void *impl;
};

typedef struct wm_service_registry_t {
  wm_service_t *items;
  size_t count;
  size_t capacity;
} wm_service_registry_t;

void wm_service_registry_init(wm_service_registry_t *registry);
void wm_service_registry_shutdown(wm_service_registry_t *registry);

bool wm_service_registry_register(wm_service_registry_t *registry,
                                  wm_service_t service);
void wm_service_registry_emit(wm_service_registry_t *registry,
                              const wm_service_event_t *event,
                              const wm_state_t *state);
