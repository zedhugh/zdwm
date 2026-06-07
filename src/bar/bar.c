#include "bar/bar.h"

#include <cairo.h>
#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/types.h"
#include "bar/workspaces.h"
#include "base/array.h"
#include "base/memory.h"

static zdwm_bar_item_t *bar_add_item(
  bar_output_t *bar_output,
  zdwm_bar_side_type_t side_type,
  zdwm_bar_item_type_t item_type,
  void *config
) {
  auto side = &bar_output->sides[side_type];
  auto item = array_push(side->items, side->count, side->capacity);
  item->api = item_type;

  auto create_state = item_type.create_state;
  if (create_state) item->state = create_state(bar_output->output_id, config);

  return item;
}

static void bar_item_cleanup(zdwm_bar_item_t *item) {
  auto destroy_state = item->api.destroy_state;
  if (destroy_state) destroy_state(item->state);
  item->state = nullptr;

  p_delete(&item->cells);
  p_clear(item, 1);
}

#define VALUE(value, fallback) ((value) ? (value) : (fallback))

void bar_output_add_workspace(
  bar_output_t *bar_output,
  zdwm_bar_config_t *config
) {
  auto side = ZDWM_BAR_SIDE_LEFT;

  bar_workspace_config_t workspace_config = {
    .cell_padding    = VALUE(config->tag_cell_padding_x, config->padding_x),
    .indicator_width = VALUE(config->tag_indicator_width, 4),
    .bg              = VALUE(config->tag_bg, config->bg),
    .fg              = VALUE(config->tag_fg, config->fg),
    .active_bg       = config->tag_active_bg,
    .active_fg       = config->tag_active_fg,
    .urgent_bg       = config->tag_urgent_bg,
    .urgent_fg       = config->tag_urgent_fg,
  };
  auto item = bar_add_item(bar_output, side, bar_workspace, &workspace_config);
  item->cell_padding    = workspace_config.cell_padding;
  item->indicator_width = workspace_config.indicator_width;
}

void bar_init(bar_t *bar) {
  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    bar_output_add_workspace(bar_output, &bar->config);
  }
}

static void bar_side_cleanup(bar_side_t *side) {
  for (size_t i = 0; i < side->count; ++i) bar_item_cleanup(&side->items[i]);

  p_delete(&side->items);
  p_clear(side, 1);
}

void bar_cleanup(bar_t *bar) {
  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    for (size_t j = 0; j < ZDWM_BAR_SIDE_COUNT; ++j) {
      auto side = &bar_output->sides[j];
      bar_side_cleanup(side);
    }
    cairo_destroy(bar_output->cr);
    bar_output->cr = nullptr;
  }
  p_delete(&bar->bars);
  bar->count = 0;
}
