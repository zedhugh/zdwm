#pragma once

#include "wm_event.h"
#include "wm_plan.h"

typedef struct wm_backend_t wm_backend_t;

typedef struct wm_backend_api_t {
  bool (*init)(wm_backend_t *backend);
  bool (*next_event)(wm_backend_t *backend, wm_event_t *out);
  bool (*apply_effect)(wm_backend_t *backend, const wm_effect_t *effect);
  void (*flush)(wm_backend_t *backend);
  void (*shutdown)(wm_backend_t *backend);
} wm_backend_api_t;

struct wm_backend_t {
  const wm_backend_api_t *api;
  void *impl;
};
