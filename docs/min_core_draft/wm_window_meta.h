#pragma once

#include "wm_types.h"

typedef struct wm_window_meta_t {
  wm_window_id_t id;
  char *title;
  char *app_id;
  char *class_name;
  char *instance_name;
} wm_window_meta_t;

typedef struct wm_window_meta_store_t {
  wm_window_meta_t *items;
  size_t count;
  size_t capacity;
} wm_window_meta_store_t;

void wm_window_meta_store_init(wm_window_meta_store_t *store);
void wm_window_meta_store_shutdown(wm_window_meta_store_t *store);

wm_window_meta_t *wm_window_meta_find(wm_window_meta_store_t *store,
                                      wm_window_id_t id);
const wm_window_meta_t *wm_window_meta_find_const(
  const wm_window_meta_store_t *store, wm_window_id_t id);

bool wm_window_meta_upsert(wm_window_meta_store_t *store,
                           const wm_window_meta_t *meta);
bool wm_window_meta_remove(wm_window_meta_store_t *store, wm_window_id_t id);
