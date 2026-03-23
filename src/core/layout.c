#include "core/layout.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

void layout_result_init(layout_result_t *result) {
  result->item_count = 0;
  result->item_capacity = INIT_CAPACITY;
  result->items = p_new(layout_item_t, result->item_capacity);
}

void layout_result_cleanup(layout_result_t *result) {
  p_delete(&result->items);
  result->item_count = 0;
  result->item_capacity = 0;
}

void layout_result_push(layout_result_t *result, layout_item_t item) {
  if (result->item_count == result->item_capacity) {
    size_t new_capacity = next_capacity(result->item_capacity);
    p_realloc(&result->items, new_capacity);
    result->item_capacity = new_capacity;
  }

  result->items[result->item_count++] = item;
}

void layout_registry_init(layout_registry_t *registry) {
  registry->slot_count = 0;
  registry->slot_capacity = INIT_CAPACITY;
  registry->slots = p_new(layout_slot_t, registry->slot_capacity);
}

void layout_registry_cleanup(layout_registry_t *registry) {
  while (registry->slot_count > 0) {
    layout_slot_t *r = &registry->slots[--registry->slot_count];
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

  if (registry->slot_count == registry->slot_capacity) {
    size_t new_capacity = next_capacity(registry->slot_capacity);
    p_realloc(&registry->slots, new_capacity);
    registry->slot_capacity = new_capacity;
  }
  layout_id_t id = (layout_id_t)registry->slot_count;

  layout_slot_t *r = &registry->slots[registry->slot_count];
  r->id = id;
  r->name = p_strdup(name);
  r->symbol = p_strdup(symbol);
  r->description = p_strdup_nullable(description);
  r->fn = fn;

  registry->slot_count++;
  return id;
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
