#include "bar/bar.h"

#include <assert.h>
#include <cairo.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "bar/binding.h"
#include "bar/cell.h"
#include "bar/text.h"
#include "bar/tray.h"
#include "bar/types.h"
#include "bar/windows.h"
#include "bar/workspaces.h"
#include "base/array.h"
#include "base/color.h"
#include "base/macros.h"
#include "base/memory.h"
#include "base/time.h"

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
    p_delete(&cell->fg_text);
    p_delete(&cell->bg_text);
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
    .cell_padding = VALUE(config->tag_cell_padding_x, config->padding_x),
    .indicator_width = VALUE(config->tag_indicator_width, 4),
    .bg = VALUE(config->tag_bg, config->bg),
    .fg = VALUE(config->tag_fg, config->fg),
    .active_bg = config->tag_active_bg,
    .active_fg = config->tag_active_fg,
    .urgent_bg = config->tag_urgent_bg,
    .urgent_fg = config->tag_urgent_fg,
    .layout_bg = VALUE(config->layout_bg, config->bg),
    .layout_fg = VALUE(config->layout_fg, config->fg),
  };
  auto item = bar_add_item(
    bar_output->output_id,
    &bar_output->left,
    bar_workspace,
    &workspace_config
  );
  item->cell_padding = workspace_config.cell_padding;
  item->indicator_width = workspace_config.indicator_width;

  bar_workspace_add_listeners(listeners, item->state);
}

static void bar_output_add_binding(
  bar_output_t *bar_output,
  zdwm_bar_config_t *config,
  listeners_t *listeners
) {
  bar_binding_config_t binding_config = {
    .show_default = config->binding_show_default,
    .cell_padding = VALUE(config->binding_padding_x, config->padding_x),
    .bg = VALUE(config->binding_mode_bg, config->bg),
    .fg = VALUE(config->binding_mode_fg, config->fg),
  };
  auto item = bar_add_item(
    bar_output->output_id,
    &bar_output->left,
    bar_binding,
    &binding_config
  );
  item->cell_padding = binding_config.cell_padding;

  bar_binding_add_listeners(listeners, item->state);
}

static void bar_output_add_windows(
  bar_output_t *bar_output,
  zdwm_bar_config_t *config,
  listeners_t *listeners
) {
  auto item = &bar_output->center;

  bar_windows_config_t window_config = {
    .cell_padding = VALUE(config->window_padding_x, config->padding_x),
    .bg = VALUE(config->window_bg, config->bg),
    .fg = VALUE(config->window_fg, config->fg),
    .focused_bg = VALUE(config->window_focused_bg, config->bg),
    .focused_fg = VALUE(config->window_focused_fg, config->fg),
  };
  item->cell_padding = window_config.cell_padding;

  item->api = bar_windows;

  auto create_state = item->api.create_state;
  auto output_id = bar_output->output_id;
  if (create_state) item->state = create_state(output_id, &window_config);

  bar_windows_add_listeners(listeners, item->state);
}

static inline void bar_output_add_tray(bar_output_t *bar_output, tray_t *tray) {
  bar_add_item(bar_output->output_id, &bar_output->right, bar_tray, tray);
}

void bar_init(bar_t *bar, listeners_t *listeners) {
  auto c = &bar->config;
  bar->ctx = text_context_create(c->font_family, c->font_size, c->dpi);
  auto tray = &bar->tray;

  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];

    bar_output->height = bar->config.height;
    bar_output_add_workspace(bar_output, &bar->config, listeners);
    bar_output_add_binding(bar_output, &bar->config, listeners);
    bar_output_add_windows(bar_output, &bar->config, listeners);

    if (tray->api.host_window(tray->handle) == bar_output->window_id) {
      bar_output_add_tray(bar_output, &bar->tray);
    }
  }

  bar->timerfd = time_create_monotonic_timerfd_by_fps(bar->config.fps);
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
}

static inline void bar_item_update(zdwm_bar_item_t *item) {
  auto update = item->api.update;
  if (!update) return;

  auto update_interval_ms = item->api.update_interval_ms;
  auto last_updated_time = item->last_updated_time;
  auto now = time_monotonic_ms();
  if (now < last_updated_time + update_interval_ms) return;

  item->last_updated_time = now;
  update(item, &bar_cell_api, item->state);
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
  auto end = bar_output->width;

  auto left = &bar_output->left;
  for (size_t i = 0; i < left->count; ++i) {
    auto item = &left->items[i];

    item->region.start = start;
    for (size_t j = 0; j < item->count; ++j) {
      auto cell = &item->cells[j];
      auto fixed_width = cell->fixed_width;

      bar_x_region_t region = {.start = start, .end = start};
      if (fixed_width == 0) {
        int32_t width = 0;
        text_context_get_text_size(ctx, cell->text, &width, nullptr);
        region.end += width + 2 * item->cell_padding;
      } else if (fixed_width > 0) {
        region.end += fixed_width;
      }
      bar_cell_set_region(item, j, region);

      start = region.end;
    }
    item->region.end = start;
  }
  if (left->count) {
    left->region.start = left->items[0].region.start;
    left->region.end = left->items[left->count - 1].region.end;
  }

  auto right = &bar_output->right;
  for (size_t i = right->count; i > 0; --i) {
    auto item_index = i - 1;
    auto item = &right->items[item_index];

    item->region.end = end;
    for (size_t j = item->count; j > 0; --j) {
      auto cell_index = j - 1;
      auto cell = &item->cells[cell_index];
      auto fixed_width = cell->fixed_width;

      bar_x_region_t region = {.start = end, .end = end};
      if (fixed_width == 0) {
        int32_t width = 0;
        text_context_get_text_size(ctx, cell->text, &width, nullptr);
        region.start -= width + item->cell_padding * 2;
      } else if (fixed_width > 0) {
        region.start -= fixed_width;
      }
      bar_cell_set_region(item, cell_index, region);

      end = region.start;
    }
    item->region.start = end;
  }
  if (right->count) {
    right->region.start = right->items[0].region.start;
    right->region.end = right->items[right->count - 1].region.end;
  }

  auto center = &bar_output->center;
  if (center->count == 0) return;
  center->region = (bar_x_region_t){.start = start, .end = end};

  auto center_width = end - start;
  auto center_count = center->count;
  for (size_t i = 0; i < center->count; ++i) {
    auto cell = &center->cells[i];
    auto fixed_width = cell->fixed_width;
    if (fixed_width > 0) {
      center_width -= fixed_width;
    } else if (fixed_width < 0) {
      /* 隐藏的 cell 不占任何空间，平分宽度时不参与 */
      --center_count;
    }
  }

  auto width = center_width / (int32_t)center_count;
  for (size_t i = 0; i < center->count; ++i) {
    auto cell = &center->cells[i];
    auto fixed_width = cell->fixed_width;

    bar_x_region_t region = {.start = start, .end = start};
    if (fixed_width == 0) {
      region.end += width + center->cell_padding * 2;
    } else if (fixed_width > 0) {
      region.end += fixed_width;
    }
    start = region.end;
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
    auto cell = &item->cells[i];
    auto region = &cell->region;

    zdwm_rect_t cell_area = {
      .x = region->start,
      .y = 0,
      .width = region->end - region->start,
      .height = height,
    };
    if (cell_area.width <= 0) {
      cell->dirty = false;
      continue;
    }

    draw_background(cr, &cell->bg, cell_area);

    if (cell->show_indicator) {
      auto size = MIN(item->indicator_width, cell_area.width);
      size = MIN(size, height);

      if (size > 0) {
        zdwm_rect_t indicator_area = {
          .x = region->start,
          .y = 0,
          .width = size,
          .height = size,
        };
        draw_background(cr, &cell->fg, indicator_area);
      }
    }

    zdwm_rect_t text_area = {
      .x = cell_area.x + item->cell_padding,
      .y = 0,
      .width = cell_area.width - item->cell_padding * 2,
      .height = height,
    };
    if (text_area.width > 0 && cell->text && cell->text[0] != '\0') {
      draw_text(cr, ctx, cell->text, &cell->fg, text_area);
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

static inline void clean_cairo_context(cairo_t *cr) {
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_restore(cr);
}

static void
bar_output_draw(bar_output_t *bar_output, text_context_t *ctx, color_t *bg) {
  auto cr = bar_output->cr;
  clean_cairo_context(cr);

  zdwm_rect_t output_area = {
    .x = 0,
    .y = 0,
    .width = bar_output->width,
    .height = bar_output->height,
  };
  draw_background(cr, bg, output_area);

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

static inline void bar_item_run_hook(
  zdwm_bar_item_t *item,
  void (*hook)(const zdwm_bar_item_hook_params_t *params)
) {
  if (!hook) return;

  zdwm_bar_item_hook_params_t params = {
    .item = item,
    .cell_api = &bar_cell_api,
    .region = item->region,
    .state = item->state
  };
  hook(&params);
}

static void visitor_after_layout(zdwm_bar_item_t *item) {
  bar_item_run_hook(item, item->api.after_layout);
}

static void visitor_after_draw(zdwm_bar_item_t *item) {
  bar_item_run_hook(item, item->api.after_draw);
}

typedef void (*bar_item_visitor_t)(zdwm_bar_item_t *item);

static void
bar_output_foreach_item(bar_output_t *bar_output, bar_item_visitor_t visit) {
  for (size_t i = 0; i < bar_output->left.count; ++i) {
    visit(&bar_output->left.items[i]);
  }
  visit(&bar_output->center);
  for (size_t i = 0; i < bar_output->right.count; ++i) {
    visit(&bar_output->right.items[i]);
  }
}

bool bar_draw(bar_t *bar) {
  auto ctx = bar->ctx;

  bool changed = false;

  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    if (!bar_output_is_dirty(bar_output)) continue;

    bar_output_layout(bar_output, ctx);
    bar_output_foreach_item(bar_output, visitor_after_layout);
    bar_output_draw(bar_output, ctx, &bar->palette.bg);
    bar_output_foreach_item(bar_output, visitor_after_draw);
    changed = true;
  }

  return changed;
}

static inline bool bar_item_click(
  zdwm_bar_item_t *item,
  bar_click_info_t info,
  zdwm_action_t *action
) {
  auto x = info.x;
  if (!(item->region.start <= x && item->region.end >= x)) return false;

  if (!item->api.on_click) return true;

  for (size_t i = 0; i < item->count; ++i) {
    auto cell = &item->cells[i];
    if (cell->region.start <= x && cell->region.end >= x) {
      zdwm_bar_click_params_t params = {
        .item = item,
        .cell_index = i,
        .x = x,
        .modifiers = info.modifiers,
        .button = info.button,
        .state = item->state,
      };
      *action = item->api.on_click(&params);
      return true;
    }
  }

  return true;
}

static inline bool
bar_side_click(bar_side_t *side, bar_click_info_t info, zdwm_action_t *action) {
  if (side->region.start > info.x || side->region.end < info.x) return false;

  for (size_t i = 0; i < side->count; ++i) {
    if (bar_item_click(&side->items[i], info, action)) return true;
  }

  return true;
}

bool bar_click(bar_t *bar, bar_click_info_t info, zdwm_action_t *action) {
  for (size_t i = 0; i < bar->count; ++i) {
    auto bar_output = &bar->bars[i];
    if (bar_output->window_id != info.window) continue;

    if (bar_side_click(&bar_output->left, info, action)) return true;
    if (bar_item_click(&bar_output->center, info, action)) return true;
    if (bar_side_click(&bar_output->right, info, action)) return true;
  }

  return false;
}
