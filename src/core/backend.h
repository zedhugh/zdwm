#pragma once

#include <stddef.h>

#include "core/wm_desc.h"

typedef struct wm_backend_t wm_backend_t;

typedef struct wm_backend_detect_t {
  wm_output_info_t *outputs;
  size_t output_count;
} wm_backend_detect_t;

wm_backend_t *wm_backend_create(const char *display_name);
void wm_backend_destroy(wm_backend_t *backend);

wm_backend_detect_t *wm_backend_detect(wm_backend_t *backend);
void wm_backend_detect_destroy(wm_backend_detect_t *detect);
