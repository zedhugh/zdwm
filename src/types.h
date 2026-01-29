#pragma once

#include <cairo.h>
#include <glib.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "base.h"
#include "color.h"

typedef struct client_t client_t;
typedef struct monitor_t monitor_t;
typedef struct tag_t tag_t;
typedef struct task_in_tag_t task_in_tag_t;

typedef struct layout_t {
  char *symbol;
  void (*arrange)(tag_t *tag);
} layout_t;

struct client_t {
  client_t *stack_next;
  client_t *next;

  monitor_t *monitor;
  uint32_t tags;

  area_t old_geometry;
  area_t geometry;
  uint16_t border_width;
  uint16_t old_border_width;
  color_t *border_color;

  bool floating;
  bool fullscreen;
  bool maximize;
  bool minimize;
  bool sticky;
  bool urgent;
  bool size_freeze; /* 尺寸冻结窗口一定是浮动窗口 */

  char *class, *instance;
  char *name, *net_name;
  char *role;

  xcb_window_t transient_for_window;
  xcb_window_t leader_window;
  xcb_window_t frame_window;
  xcb_window_t window;
};

struct monitor_t {
  monitor_t *next;
  area_t geometry;
  area_t workarea;
  tag_t *tag_list;
  tag_t *selected_tag;

  char *name;
  cairo_t *bar_cr;

  extent_in_bar_t tag_extent;
  extent_in_bar_t layout_symbol_extent;

  xcb_window_t bar_window;

  point_t cursor_position;
  bool position_inited;
};

struct tag_t {
  uint32_t index; /* index in all tags */
  uint32_t mask;
  extent_in_bar_t bar_extent;
  tag_t *next;
  char *name;
  const layout_t *layout;
  task_in_tag_t *task_list;
};

struct task_in_tag_t {
  client_t *client;
  task_in_tag_t *next;
  area_t geometry;
  extent_in_bar_t bar_extent;
};

typedef struct color_set_t {
  color_t bar_bg;
  color_t tag_bg;
  color_t active_tag_bg;
  color_t tag_color;
  color_t active_tag_color;
  color_t border_color;
  color_t active_border_color;
} color_set_t;

typedef struct padding_t {
  uint16_t bar_y;
  uint16_t tag_x;
} padding_t;
