#include "bar/bar.h"

#include <assert.h>
#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
#include <cairo.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/cell.h"
#include "bar/text.h"
#include "bar/types.h"
#include "bar/workspaces.h"
#include "base/array.h"
#include "base/macros.h"
#include "base/memory.h"
#include "core/listeners.h"

static zdwm_bar_item_t *bar_add_item(
  zdwm_output_id_t output_id,
  bar_side_t *side,
  zdwm_bar_item_type_t item_type,
  void *config
) {
  auto item = array_push(side->items, side->count, side->capacity);
  item->api = item_type;

  auto create_state = item_type.create_state;
  if (create_state) item->state = create_state(output_id, config);

  return item;
}

static void bar_item_cleanup(zdwm_bar_item_t *item) {
  auto destroy_state = item->api.destroy_state;
  if (destroy_state) destroy_state(item->state);
  item->state = nullptr;

  for (size_t i = 0; i < item->count; ++i) {
    auto cell = &item->cells[i];
    p_delete(&cell->text);
  }

  p_delete(&item->cells);
  p_clear(item, 1);
}

#define VALUE(value, fallback) ((value) ? (value) : (fallback))

void bar_output_add_workspace(
  bar_output_t *bar_output,
  zdwm_bar_config_t *config,
  listeners_t *listeners
) {
  bar_workspace_config_t workspace_config = {
    .cell_padding    = VALUE(config->tag_cell_padding_x, config->padding_x),
    .indicator_width = VALUE(config->tag_indicator_width, 4),
    .bg              = VALUE(config->tag_bg, config->bg),
    .fg              = VALUE(config->tag_fg, config->fg),
    .active_bg       = config->tag_active_bg,
    .active_fg       = config->tag_active_fg,
    .urgent_bg       = config->tag_urgent_bg,
    .urgent_fg       = config->tag_urgent_fg,
    .layout_bg       = config->layout_bg,
    .layout_fg       = config->layout_fg,
  };
  auto item = bar_add_item(
    bar_output->output_id,
    &bar_output->left,
    bar_workspace,
    &workspace_config
  );
  item->cell_padding    = workspace_config.cell_padding;
  item->indicator_width = workspace_config.indicator_width;

  bar_workspace_add_listeners(listeners, item->state);
}

void bar_init(bar_t *bar, listeners_t *listeners) {
  auto c   = &bar->config;
  bar->ctx = text_context_create(c->font_family, c->font_size, c->dpi);

  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];

    bar_output->height = bar->config.height;
    bar_output_add_workspace(bar_output, &bar->config, listeners);
  }

  bar->timerfd     = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
  auto interval_ns = 1'000'000'000ULL / bar->config.fps;

  struct itimerspec timer_spec = {
    .it_interval =
      {
        .tv_sec  = interval_ns / 1'000'000'000,
        .tv_nsec = interval_ns % 1'000'000'000,
      },
    .it_value = {
      .tv_sec  = 0,
      .tv_nsec = interval_ns % 1'000'000'000,
    },
  };
  timerfd_settime(bar->timerfd, 0, &timer_spec, nullptr);
}

static void bar_side_cleanup(bar_side_t *side) {
  for (size_t i = 0; i < side->count; ++i) bar_item_cleanup(&side->items[i]);

  p_delete(&side->items);
  p_clear(side, 1);
}

void bar_cleanup(bar_t *bar) {
  close(bar->timerfd);
  bar->timerfd = -1;

  text_context_destory(bar->ctx);
  bar->ctx = nullptr;

  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    bar_side_cleanup(&bar_output->left);
    bar_side_cleanup(&bar_output->right);
    bar_item_cleanup(&bar_output->center);

    cairo_destroy(bar_output->cr);
    bar_output->cr = nullptr;
  }
  p_delete(&bar->bars);
  bar->count = 0;

  bar_cell_clean_color_cache();
}

static inline void bar_item_update(zdwm_bar_item_t *item) {
  auto update = item->api.update;
  if (update) update(item, &bar_cell_api, item->state);
}

static inline void bar_side_update(bar_side_t *side) {
  for (size_t i = 0; i < side->count; ++i) {
    bar_item_update(&side->items[i]);
  }
}

void bar_update(bar_t *bar) {
  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    bar_side_update(&bar_output->left);
    bar_side_update(&bar_output->right);
    bar_item_update(&bar_output->center);
  }
}

static void bar_output_layout(bar_output_t *bar_output, text_context_t *ctx) {
  int32_t start = 0;
  auto end      = bar_output->width;

  auto left = &bar_output->left;
  for (size_t i = 0; i < left->count; ++i) {
    auto item = &left->items[i];

    item->region.start = start;
    for (size_t j = 0; j < item->count; ++j) {
      auto cell = &item->cells[j];

      int32_t width = 0;
      text_context_get_text_size(ctx, cell->text, &width, nullptr);

      bar_x_region_t region = {
        .start = start,
        .end   = start + width + item->cell_padding * 2,
      };
      bar_cell_set_region(item, j, region);

      start = region.end;
    }
    item->region.end = start;
  }

  auto right = &bar_output->right;
  for (size_t i = right->count; i > 0; --i) {
    auto item_index = i - 1;
    auto item       = &right->items[item_index];

    item->region.end = end;
    for (size_t j = item->count; j > 0; --j) {
      auto cell_index = j - 1;
      auto cell       = &item->cells[cell_index];

      int32_t width = 0;
      text_context_get_text_size(ctx, cell->text, &width, nullptr);

      bar_x_region_t region = {
        .start = end - width - item->cell_padding * 2,
        .end   = end,
      };
      bar_cell_set_region(item, cell_index, region);

      end = region.start;
    }
    item->region.start = end;
  }

  auto center = &bar_output->center;
  if (center->count == 0) return;
  center->region = (bar_x_region_t){.start = start, .end = end};

  auto width = (end - start) / (int32_t)center->count;
  for (size_t i = 0; i < center->count; ++i) {
    bar_x_region_t region = {
      .start = start,
      .end   = start + width + center->cell_padding * 2,
    };
    bar_cell_set_region(center, i, region);
  }
}

static void bar_item_draw(
  cairo_t *cr,
  zdwm_bar_item_t *item,
  text_context_t *ctx,
  int32_t height
) {
  for (size_t i = 0; i < item->count; ++i) {
    auto cell   = &item->cells[i];
    auto region = &cell->region;

    zdwm_rect_t cell_area = {
      .x      = region->start,
      .y      = 0,
      .width  = region->end - region->start,
      .height = height,
    };
    if (cell_area.width <= 0) {
      cell->dirty = false;
      continue;
    }

    draw_background(cr, cell->bg, cell_area);

    if (cell->show_indicator) {
      auto size = MIN(item->indicator_width, cell_area.width);
      size      = MIN(size, height);

      if (size > 0) {
        zdwm_rect_t indicator_area = {
          .x      = region->start,
          .y      = 0,
          .width  = size,
          .height = size,
        };
        draw_background(cr, cell->fg, indicator_area);
      }
    }

    zdwm_rect_t text_area = {
      .x      = cell_area.x + item->cell_padding,
      .y      = 0,
      .width  = cell_area.width - item->cell_padding * 2,
      .height = height,
    };
    if (text_area.width > 0) {
      draw_text(cr, ctx, cell->text, cell->fg, text_area);
    }

    cell->dirty = false;
  }

  item->dirty = false;
}

static bool bar_item_is_dirty(zdwm_bar_item_t *item) {
  if (item->dirty) return true;

  for (size_t i = 0; i < item->count; ++i) {
    auto cell = &item->cells[i];
    if (cell->dirty) return true;
  }

  return false;
}

static bool bar_side_is_dirty(bar_side_t *side) {
  for (size_t j = 0; j < side->count; ++j) {
    auto item = &side->items[j];
    if (bar_item_is_dirty(item)) return true;
  }
  return false;
}

static inline bool bar_output_is_dirty(bar_output_t *bar_output) {
  if (bar_side_is_dirty(&bar_output->left)) return true;
  if (bar_side_is_dirty(&bar_output->right)) return true;
  if (bar_item_is_dirty(&bar_output->center)) return true;
  return false;
}

static void bar_output_draw(bar_output_t *bar_output, text_context_t *ctx) {
  auto cr = bar_output->cr;

  auto left = &bar_output->left;
  for (size_t i = 0; i < left->count; ++i) {
    bar_item_draw(cr, &left->items[i], ctx, bar_output->height);
  }
  auto right = &bar_output->right;
  for (size_t i = 0; i < right->count; ++i) {
    bar_item_draw(cr, &right->items[i], ctx, bar_output->height);
  }

  bar_item_draw(cr, &bar_output->center, ctx, bar_output->height);
}

bool bar_draw(bar_t *bar) {
  auto ctx = bar->ctx;

  bool changed = false;

  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    if (!bar_output_is_dirty(bar_output)) continue;

    bar_output_layout(bar_output, ctx);
    bar_output_draw(bar_output, ctx);
    changed = true;
  }

  return changed;
}
