#include "bar/cell.h"

#include <assert.h>
#include <stddef.h>
#include <strings.h>
#include <zdwm/types.h>

#include "bar/types.h"
#include "base/array.h"
#include "base/color.h"
#include "base/memory.h"

typedef struct color_cache_item_t {
  const char *text;
  color_t color;
} color_cache_item_t;

typedef struct color_cache_t {
  color_cache_item_t *items;
  size_t count;
  size_t capacity;
} color_cache_t;

static color_cache_t cache = {};

static color_t *find_or_insert_color(const char *text) {
  assert(text);
  for (size_t i = 0; i < cache.count; ++i) {
    auto item = &cache.items[i];
    if (text == item->text || strcasecmp(text, item->text) == 0) {
      return &item->color;
    }
  }

  auto slot  = array_push(cache.items, cache.count, cache.capacity);
  slot->text = p_strdup(text);
  color_parse(text, &slot->color);
  return &slot->color;
}

void bar_cell_reset_color_cache(void) {
  for (size_t i = 0; i < cache.count; ++i) {
    auto item = &cache.items[i];
    p_delete(&item->text);
  }
  p_clear(cache.items, cache.count);
  cache.count = 0;
}

void bar_cell_clean_color_cache(void) {
  bar_cell_reset_color_cache();
  p_delete(&cache.items);
  cache.capacity = 0;
}

static size_t bar_cell_get_count(zdwm_bar_item_t *item) { return item->count; }

static void bar_cell_set_count(zdwm_bar_item_t *item, size_t count) {
  if (item->count == count) return;

  for (size_t i = 0; i < item->count; ++i) p_delete(&item->cells[i].text);

  item->count = count;
  p_realloc(&item->cells, count);
  p_clear(item->cells, count);
  item->dirty = true;
}

static void
bar_cell_set_text(zdwm_bar_item_t *item, size_t index, const char *text) {
  if (index >= item->count) return;

  auto cell = &item->cells[index];
  if (cell->text && text && strcmp(cell->text, text) == 0) return;

  p_delete(&cell->text);

  cell->text = p_strdup_nullable(text);

  cell->dirty = true;
  item->dirty = true;
}

static void
bar_cell_set_bg(zdwm_bar_item_t *item, size_t index, const char *color) {
  if (index >= item->count) return;

  auto cell         = &item->cells[index];
  auto cached_color = find_or_insert_color(color);
  if (cell->bg == cached_color) return;

  cell->bg = cached_color;

  cell->dirty = true;
  item->dirty = true;
}

static void
bar_cell_set_fg(zdwm_bar_item_t *item, size_t index, const char *color) {
  if (index >= item->count) return;

  auto cell         = &item->cells[index];
  auto cached_color = find_or_insert_color(color);
  if (cell->fg == cached_color) return;

  cell->fg = cached_color;

  cell->dirty = true;
  item->dirty = true;
}

static void
bar_cell_set_icon(zdwm_bar_item_t *item, size_t index, zdwm_icon_t icon) {
  if (index >= item->count) return;

  auto cell     = &item->cells[index];
  auto icon_ptr = &cell->icon;

  if (icon_ptr->type == icon.type) {
    switch (icon.type) {
    case ZDWM_ICON_TEXT:
      if (icon_ptr->as.text == icon.as.text ||
          strcmp(icon_ptr->as.text, icon.as.text) == 0) {
        return;
      }
    case ZDWM_ICON_IMAGE:
      if (icon_ptr->as.image_path == icon.as.image_path ||
          strcmp(icon_ptr->as.image_path, icon.as.image_path) == 0) {
        return;
      }
    }
  }

  *icon_ptr = icon;

  cell->dirty = true;
  item->dirty = true;
}

static void
bar_cell_set_indicator(zdwm_bar_item_t *item, size_t index, bool show) {
  if (index >= item->count) return;

  auto cell = &item->cells[index];
  if (cell->show_indicator == show) return;

  cell->show_indicator = show;

  cell->dirty = true;
  item->dirty = true;
}

zdwm_bar_cell_api_t bar_cell_api = {
  .get_cell_count     = bar_cell_get_count,
  .set_cell_count     = bar_cell_set_count,
  .cell_set_text      = bar_cell_set_text,
  .cell_set_bg        = bar_cell_set_bg,
  .cell_set_fg        = bar_cell_set_fg,
  .cell_set_icon      = bar_cell_set_icon,
  .cell_set_indicator = bar_cell_set_indicator,
};

void bar_cell_set_region(
  zdwm_bar_item_t *item,
  size_t index,
  bar_x_region_t region
) {
  if (index >= item->count) return;

  auto cell = &item->cells[index];
  auto r    = &cell->region;

  if (r->start == region.start && r->end == region.end) return;

  *r = region;

  cell->dirty = true;
  item->dirty = true;
}
