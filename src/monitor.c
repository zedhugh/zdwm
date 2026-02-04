#include "monitor.h"

#include <cairo-xcb.h>
#include <cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>

#include "app.h"
#include "base.h"
#include "client.h"
#include "color.h"
#include "status.h"
#include "text.h"
#include "types.h"
#include "utils.h"
#include "wm.h"
#include "xcursor.h"
#include "xwindow.h"

uint32_t monitor_initialize_tag(monitor_t *monitor, const char **tags,
                                uint32_t tag_index_start_at) {
  uint32_t tag_count = 0;
  int i = 0;
  const char *tag_name = nullptr;
  tag_t *prev_tag = nullptr;
  for (tag_name = tags[i]; tag_name; tag_name = tags[++i]) {
    tag_t *tag = p_new(tag_t, 1);
    tag->name = strdup(tag_name);
    tag->mask = 1u << i;
    tag->index = tag_index_start_at + tag_count;

    if (wm.layout_count > 0 && wm.layout_list) {
      tag->layout = &wm.layout_list[0];
    }

    if (prev_tag) {
      prev_tag->next = tag;
    } else {
      monitor->tag_list = tag;
    }

    prev_tag = tag;
    tag_count++;
  }

  monitor->selected_tag = monitor->tag_list;
  return tag_count;
}

void monitor_deal_focus(monitor_t *monitor) {
  if (!monitor->selected_tag->task_list) {
    client_focus(nullptr);
    return;
  }

  client_t *client = nullptr;
  for (client_t *c = wm.client_stack_list; c; c = c->stack_next) {
    if (client_get_task_in_tag(c, monitor->selected_tag)) {
      client = c;
      break;
    }
  }
  client_focus(client);
}

void monitor_select_tag(monitor_t *monitor, uint32_t tag_mask) {
  if (monitor->selected_tag->mask == tag_mask) return;

  for (task_in_tag_t *task = monitor->selected_tag->task_list; task;
       task = task->next) {
    if (task->client->floating || task->client->size_freeze ||
        !monitor->selected_tag->layout->arrange) {
      task->geometry = task->client->geometry;
      task->client->old_geometry = task->client->geometry;
    }
    if (task->client->fullscreen || task->client->maximize ||
        task->client->minimize) {
      task->geometry = task->client->geometry;
    }
  }

  for (tag_t *tag = monitor->tag_list; tag; tag = tag->next) {
    if (tag->mask == tag_mask) {
      monitor->selected_tag = tag;
      monitor_arrange(monitor);
      monitor_draw_bar(monitor);
      break;
    }
  }

  monitor_deal_focus(monitor);
  xcb_flush(wm.xcb_conn);
}

static void tag_clean(tag_t *tag) {
  tag_t *next_tag = nullptr;
  for (tag_t *t = tag; t; t = next_tag) {
    p_delete(&t->name);

    next_tag = t->next;
    p_delete(&t);
  }
}

void monitor_clean(monitor_t *monitor) {
  monitor_t *next_monitor = nullptr;
  for (monitor_t *m = monitor; m; m = next_monitor) {
    p_delete(&m->name);
    tag_clean(m->tag_list);

    m->selected_tag = nullptr;
    m->tag_list = nullptr;

    cairo_destroy(m->bar_cr);
    m->bar_cr = nullptr;

    next_monitor = m->next;
    p_delete(&m);
  }
}

void monitor_init_bar(monitor_t *monitor) {
  static xcb_colormap_t colormap = XCB_NONE;
  visual_t *visual = xwindow_get_xcb_visual(true);
  xcb_visualid_t visual_id = visual->visual->visual_id;
  xcb_connection_t *conn = wm.xcb_conn;
  if (colormap == XCB_NONE) {
    colormap = xcb_generate_id(conn);
    uint8_t alloc = XCB_COLORMAP_ALLOC_NONE;
    xcb_create_colormap(conn, alloc, colormap, wm.screen->root, visual_id);
  }

  xcb_window_t window = xcb_generate_id(conn);
  uint32_t value_mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_BACK_PIXEL |
                        XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
                        XCB_CW_CURSOR | XCB_CW_COLORMAP;
  const xcb_create_window_value_list_t value_list = {
    .override_redirect = true,
    .background_pixel = wm.color_set.bar_bg.argb,
    .border_pixel = 0,
    .event_mask = XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE,
    .cursor = xcursor_get_xcb_cursor(cursor_normal),
    .colormap = colormap,
  };
  xcb_void_cookie_t cookie;
  cookie = xcb_create_window_aux_checked(
    conn, visual->depth, window, wm.screen->root, monitor->geometry.x,
    monitor->geometry.y, monitor->geometry.width, wm.bar_height, 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT, visual_id, value_mask, &value_list);
  if (xcb_request_check(conn, cookie)) fatal("cannot create bar window");

  cookie = xcb_map_window(conn, window);
  if (xcb_request_check(conn, cookie)) fatal("cannot map bar window:");

  xwindow_set_class_instance(window);
  xwindow_set_name_static(window, APP_NAME "_bar");

  xcb_aux_sync(conn);

  uint16_t width = monitor->geometry.width;
  monitor->workarea.x = monitor->geometry.x;
  monitor->workarea.y = monitor->geometry.y + wm.bar_height;
  monitor->workarea.width = width;
  monitor->workarea.height = monitor->geometry.height - wm.bar_height;
  monitor->bar_window = window;
  cairo_surface_t *surface = cairo_xcb_surface_create(
    wm.xcb_conn, window, visual->visual, width, wm.bar_height);
  cairo_t *cr = cairo_create(surface);
  cairo_surface_destroy(surface);
  p_delete(&visual);

  cairo_status_t status = cairo_status(cr);
  if (status != CAIRO_STATUS_SUCCESS) {
    fatal("cannot create cairo context: %s", cairo_status_to_string(status));
  }
  monitor->bar_cr = cr;
}

static void monitor_draw_tags(monitor_t *monitor) {
  int16_t x = 0;
  for (tag_t *tag = monitor->tag_list; tag; tag = tag->next) {
    bool selected = monitor->selected_tag == tag;
    bool has_client = tag->task_list;
    color_t *bg = nullptr;
    color_t *color = nullptr;
    if (selected) {
      bg = &wm.color_set.active_tag_bg;
      color = &wm.color_set.active_tag_color;
    } else {
      bg = &wm.color_set.tag_bg;
      color = &wm.color_set.tag_color;
    }
    int width = 0, height = 0;
    text_get_size(tag->name, &width, &height);
    width += 2 * wm.padding.tag_x;
    tag->bar_extent.start = x;
    tag->bar_extent.end = x + width;

    area_t tag_rect = {.x = x, .y = 0, .width = width, .height = wm.bar_height};
    if (bg->rgba != wm.color_set.bar_bg.rgba) {
      draw_background(monitor->bar_cr, bg, tag_rect);
    }
    if (has_client) {
      area_t rect = {.x = x, .y = 0, .width = 4, .height = 4};
      draw_rect(monitor->bar_cr, rect, selected, color, 1);
    }
    area_t text_rect = tag_rect;
    tag_rect.y = (int16_t)((int)wm.bar_height - height) / 2;
    tag_rect.height = (int16_t)height;
    draw_text(monitor->bar_cr, tag->name, color, text_rect, true);
    x += width;
  }

  monitor->tag_extent.start = 0;
  monitor->tag_extent.end = x;
}

static void monitor_draw_layout_symbol(monitor_t *monitor) {
  const layout_t *layout = monitor->selected_tag->layout;
  if (layout && layout->symbol) {
    color_t *color = &wm.color_set.tag_color;
    int width = 0;
    text_get_size(layout->symbol, &width, nullptr);
    area_t area = {
      .x = monitor->tag_extent.end,
      .y = monitor->geometry.y,
      .width = width + 2 * wm.padding.tag_x,
      .height = wm.bar_height,
    };
    monitor->layout_symbol_extent.start = area.x;
    monitor->layout_symbol_extent.end = area.x + area.width;
    draw_text(monitor->bar_cr, layout->symbol, color, area, true);
  }
}

static void monitor_draw_status(monitor_t *monitor, status_t *status) {
  monitor->status_extent.end = monitor->geometry.width;
  monitor->status_extent.start = monitor->geometry.width;
  int16_t end = monitor->layout_symbol_extent.end;

  if (status == nullptr) return;

  char cpu[16], mem[64];
  snprintf(cpu, sizeof(cpu), "%.1lf", status->cpu_usage_percent);
  snprintf(mem, sizeof(mem), "%s(%.1f%%)", status->mem_usage.mem_used_text,
           status->mem_usage.mem_percent);
  char *status_items[] = {
    status->net_speed.down, status->net_speed.up, mem, cpu, status->time,
  };
  int16_t start = monitor->status_extent.start;
  for (int i = countof(status_items) - 1; i >= 0; i--) {
    const char *text = status_items[i];
    if (!text || !strlen(text)) continue;

    int width = 0;
    text_get_size(text, &width, nullptr);
    if (!width) continue;

    start -= width;
    if (start <= end) return;
    color_t *color = &wm.color_set.tag_color;
    area_t rect = {.x = start, .y = 0, .width = width, .height = wm.bar_height};
    draw_text(monitor->bar_cr, text, color, rect, true);
    monitor->status_extent.start = start;

    start -= wm.padding.tag_x;
    if (start < end) return;
  }
}

static void monitor_draw_tasks(monitor_t *monitor) {
  task_in_tag_t *task_list = monitor->selected_tag->task_list;
  if (!task_list) return;

  uint16_t task_count = 0;
  for (task_in_tag_t *task = task_list; task; task = task->next) ++task_count;
  if (task_count == 0) return;

  int16_t x = monitor->layout_symbol_extent.end;
  uint16_t task_width = (monitor->status_extent.start - x) / task_count;
  if (task_width < wm.font_size) return;

  for (task_in_tag_t *task = task_list; task; task = task->next) {
    task->bar_extent.start = x;
    task->bar_extent.end = x + task_width;
    x += task_width;

    char **title = client_get_task_title(task->client);
    if (title == nullptr || *title == nullptr) continue;
    color_t *color = &wm.color_set.active_tag_color;
    area_t area = {
      .x = task->bar_extent.start,
      .y = monitor->geometry.y,
      .width = task_width,
      .height = wm.bar_height,
    };
    draw_text(monitor->bar_cr, *title, color, area, false);
  }
}

void monitor_draw_bar(monitor_t *monitor) {
  cairo_t *cr = monitor->bar_cr;
  uint16_t height = wm.bar_height;
  area_t bar_area = {
    .x = 0,
    .y = 0,
    .width = monitor->geometry.width,
    .height = height,
  };
  draw_background(cr, &wm.color_set.bar_bg, bar_area);

  monitor_draw_tags(monitor);
  monitor_draw_layout_symbol(monitor);
  monitor_draw_status(monitor, wm.status);
  monitor_draw_tasks(monitor);
}

void monitor_arrange(monitor_t *monitor) {
  tag_t *tag = monitor->selected_tag;
  if (tag->layout && tag->layout->arrange) tag->layout->arrange(tag);

  for (client_t *c = wm.client_stack_list; c; c = c->stack_next) {
    if (c->monitor != monitor || c->minimize) continue;

    task_in_tag_t *task = client_get_task_in_tag(c, tag);
    if (task) {
      if (client_need_layout(c)) {
        client_apply_geometry(c, task->geometry);
      } else if (c->fullscreen) {
        client_apply_geometry(c, c->monitor->geometry);
      } else if (c->maximize) {
        client_apply_geometry(c, c->monitor->workarea);
      } else {
        client_move_to(c, task->geometry.x, task->geometry.y);
      }
    } else {
      int16_t x = -client_width(c);
      int16_t y = -client_height(c);
      client_move_to(c, x, y);
    }
  }
  xcb_flush(wm.xcb_conn);
}

void monitor_save_cursor_point(monitor_t *monitor) {
  monitor->cursor_position = xcursor_query_pointer_position();
  monitor->position_inited = true;
}

void monitor_restore_cursor_point(monitor_t *monitor) {
  if (monitor->position_inited) {
    xcursor_set_pointer_position(monitor->cursor_position);
    return;
  }

  point_t point = xcursor_query_pointer_position();
  monitor_t *m = wm_get_monitor_by_point(point);
  monitor->cursor_position.x = point.x - m->geometry.x + monitor->geometry.x;
  monitor->cursor_position.y = point.y - m->geometry.y + monitor->geometry.y;
  monitor->position_inited = true;
}
