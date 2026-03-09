#pragma once

#include "wm_types.h"

typedef struct wm_layout_window_ref_t {
  wm_window_id_t window_id;
  wm_window_geometry_mode_t geometry_mode;
} wm_layout_window_ref_t;

typedef struct wm_layout_ctx_t {
  wm_output_id_t output_id;
  wm_workspace_id_t workspace_id;
  wm_rect_t workarea;
  const wm_layout_window_ref_t *windows;
  size_t window_count;
} wm_layout_ctx_t;

typedef struct wm_layout_item_t {
  wm_window_id_t window_id;
  wm_rect_t rect;
} wm_layout_item_t;

typedef struct wm_layout_result_t {
  wm_layout_item_t *items;
  size_t item_count;
  size_t item_capacity;
} wm_layout_result_t;

typedef bool (*wm_layout_fn)(const wm_layout_ctx_t *ctx,
                             wm_layout_result_t *out);

typedef struct wm_layout_slot_t {
  wm_layout_id_t id;
  const char *name;
  const char *symbol_text;
  const char *symbol_icon_path;
  wm_layout_fn fn;
} wm_layout_slot_t;

typedef struct wm_layout_registry_t {
  wm_layout_slot_t *slots;
  size_t slot_count;
  size_t slot_capacity;
} wm_layout_registry_t;

void wm_layout_result_init(wm_layout_result_t *result);
void wm_layout_result_reset(wm_layout_result_t *result);
void wm_layout_result_shutdown(wm_layout_result_t *result);

bool wm_layout_result_push(wm_layout_result_t *result, wm_layout_item_t item);

void wm_layout_registry_init(wm_layout_registry_t *registry);
void wm_layout_registry_shutdown(wm_layout_registry_t *registry);

bool wm_layout_register(wm_layout_registry_t *registry, wm_layout_id_t id,
                        wm_layout_fn fn);
wm_layout_fn wm_layout_lookup(const wm_layout_registry_t *registry,
                              wm_layout_id_t id);
