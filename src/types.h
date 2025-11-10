#pragma once

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

  area_t geometry;
  color_t *border_color;
  uint16_t border_width;

  bool floating;
  bool fullscreen;
  bool maximize;
  bool minimize;
  bool sticky;
  bool urgent;

  char *class, *instance;
  char *name, *icon_name, *net_name, *net_icon_name;
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

  xcb_window_t bar_window;
};

struct tag_t {
  uint32_t index; /* index in all tags */
  uint32_t mask;
  extent_in_bar_t bar_extent;
  tag_t *next;
  char *name;
  layout_t *layout;
  task_in_tag_t *task_list;
};

struct task_in_tag_t {
  client_t *client;
  task_in_tag_t *next;
  area_t geometry;
  extent_in_bar_t bar_extent;
};
