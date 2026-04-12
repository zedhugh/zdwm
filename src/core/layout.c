#include "core/layout.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "base/array.h"
#include "base/memory.h"
#include "core/types.h"

void layout_result_cleanup(layout_result_t *result) {
  p_delete(&result->items);
  result->item_count = 0;
  result->item_capacity = 0;
}

void layout_result_push(layout_result_t *result, layout_item_t item) {
  layout_item_t *slot =
    array_push(result->items, result->item_count, result->item_capacity);
  *slot = item;
}

void layout_registry_cleanup(layout_registry_t *registry) {
  for (size_t i = 0; i < registry->slot_count; ++i) {
    layout_slot_t *r = &registry->slots[i];
    p_delete(&r->name);
    p_delete(&r->symbol);
    p_delete(&r->description);
    r->fn = nullptr;
    r->id = ZDWM_LAYOUT_ID_INVALID;
  }

  p_delete(&registry->slots);
  registry->slot_count = 0;
  registry->slot_capacity = 0;
}

bool layout_registry_move(layout_registry_t *src, layout_registry_t *dest) {
  if (!src || !dest) return false;

  dest->slots = src->slots;
  dest->slot_count = src->slot_count;
  dest->slot_capacity = src->slot_capacity;

  src->slots = nullptr;
  src->slot_count = 0;
  src->slot_capacity = 0;

  return true;
}

size_t layout_registry_count(const layout_registry_t *registry) {
  return registry->slot_count;
}

const layout_slot_t *layout_registry_at(const layout_registry_t *registry,
                                        size_t index) {
  if (index >= registry->slot_count) return nullptr;
  return &registry->slots[index];
}

layout_id_t layout_register(layout_registry_t *registry, const char *name,
                            const char *symbol, const char *description,
                            layout_fn fn) {
  if (!name || !symbol) return ZDWM_LAYOUT_ID_INVALID;

  layout_id_t id = (layout_id_t)registry->slot_count;
  layout_slot_t *r =
    array_push(registry->slots, registry->slot_count, registry->slot_capacity);
  r->id = id;
  r->name = p_strdup(name);
  r->symbol = p_strdup(symbol);
  r->description = p_strdup_nullable(description);
  r->fn = fn;

  return r->id;
}

layout_fn layout_get(const layout_registry_t *registry, layout_id_t id) {
  const layout_slot_t *layout_slot = layout_slot_get(registry, id);
  if (layout_slot) return layout_slot->fn;
  return nullptr;
}

const layout_slot_t *layout_slot_get(const layout_registry_t *registry,
                                     layout_id_t id) {
  if (id >= registry->slot_count) return nullptr;
  return &registry->slots[id];
}
