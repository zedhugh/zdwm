#include "core/layout.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

void wm_layout_result_init(wm_layout_result_t *result) {
  result->item_count = 0;
  result->item_capacity = INIT_CAPACITY;
  result->items = p_new(wm_layout_item_t, result->item_capacity);
}

void wm_layout_result_reset(wm_layout_result_t *result) {
  result->item_count = 0;
}

void wm_layout_result_cleanup(wm_layout_result_t *result) {
  p_delete(&result->items);
  result->item_count = 0;
  result->item_capacity = 0;
}

void wm_layout_result_push(wm_layout_result_t *result, wm_layout_item_t item) {
  if (result->item_count == result->item_capacity) {
    size_t new_capacity = next_capacity(result->item_capacity);
    p_realloc(&result->items, new_capacity);
    result->item_capacity = new_capacity;
  }

  result->items[result->item_count++] = item;
}

void wm_layout_registry_init(wm_layout_registry_t *registry) {
  registry->slot_count = 0;
  registry->slot_capacity = INIT_CAPACITY;
  registry->slots = p_new(wm_layout_slot_t, registry->slot_capacity);
}

void wm_layout_registry_cleanup(wm_layout_registry_t *registry) {
  while (registry->slot_count > 0) {
    wm_layout_slot_t *r = &registry->slots[--registry->slot_count];
    p_delete(&r->name);
    p_delete(&r->symbol);
    p_delete(&r->description);
    r->fn = nullptr;
    r->id = WM_LAYOUT_ID_INVALID;
  }
  p_delete(&registry->slots);
  registry->slot_count = 0;
  registry->slot_capacity = 0;
}

bool wm_layout_registry_move(wm_layout_registry_t *src,
                             wm_layout_registry_t *dest) {
  if (!src || !dest) return false;

  dest->slots = src->slots;
  dest->slot_count = src->slot_count;
  dest->slot_capacity = src->slot_capacity;

  src->slots = nullptr;
  src->slot_count = 0;
  src->slot_capacity = 0;

  return true;
}

size_t wm_layout_registry_count(const wm_layout_registry_t *registry) {
  return registry->slot_count;
}

const wm_layout_slot_t *wm_layout_registry_at(
  const wm_layout_registry_t *registry, size_t index) {
  if (index >= registry->slot_count) return nullptr;
  return &registry->slots[index];
}

wm_layout_id_t wm_layout_register(wm_layout_registry_t *registry,
                                  const char *name, const char *symbol,
                                  const char *description, wm_layout_fn fn) {
  if (!name || !symbol) return WM_LAYOUT_ID_INVALID;

  if (registry->slot_count == registry->slot_capacity) {
    size_t new_capacity = next_capacity(registry->slot_capacity);
    p_realloc(&registry->slots, new_capacity);
    registry->slot_capacity = new_capacity;
  }
  wm_layout_id_t id = (wm_layout_id_t)registry->slot_count;

  wm_layout_slot_t *r = &registry->slots[registry->slot_count];
  r->id = id;
  r->name = p_strdup(name);
  r->symbol = p_strdup(symbol);
  r->description = description ? p_strdup(description) : nullptr;
  r->fn = fn;

  registry->slot_count++;
  return id;
}

wm_layout_fn wm_layout_get(const wm_layout_registry_t *registry,
                           wm_layout_id_t id) {
  const wm_layout_slot_t *layout_slot = wm_layout_slot_get(registry, id);
  if (layout_slot) return layout_slot->fn;
  return nullptr;
}

const wm_layout_slot_t *wm_layout_slot_get(const wm_layout_registry_t *registry,
                                           wm_layout_id_t id) {
  if (id >= registry->slot_count) return nullptr;
  return &registry->slots[id];
}
