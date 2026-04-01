#pragma once

#include <stddef.h>

#include "core/event.h"
#include "core/types.h"

typedef struct backend_t backend_t;

typedef struct backend_detect_t {
  output_info_t *outputs;
  size_t output_count;
} backend_detect_t;

backend_t *backend_create(const char *display_name);
void backend_destroy(backend_t *backend);

backend_detect_t *backend_detect(backend_t *backend);
void backend_detect_destroy(backend_detect_t *detect);

bool backend_next_event(backend_t *backend, event_t *event);
