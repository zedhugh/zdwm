#include "client.h"

#include <stdint.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "atoms-extern.h"
#include "monitor.h"
#include "types.h"
#include "utils.h"
#include "wm.h"
#include "xwindow.h"

client_t *client_get_by_window(xcb_window_t window) {
  for (client_t *c = wm.client_list; c; c = c->next) {
    if (c->window == window) return c;
  }

  return nullptr;
}

static void client_attach_list(client_t *client) {
  client->next = wm.client_list;
  wm.client_list = client;
}

static void client_detach_list(client_t *client) {
  client_t **tc = nullptr;
  for (tc = &wm.client_list; *tc && *tc != client; tc = &(*tc)->next);
  *tc = client->next;
}

static void client_attach_stack(client_t *client) {
  client->stack_next = wm.client_stack_list;
  wm.client_stack_list = client;
}

static void client_detach_stack(client_t *client) {
  client_t **tc = &wm.client_stack_list;
  for (; *tc && *tc != client; tc = &(*tc)->stack_next);
  *tc = client->stack_next;

  if (client == wm.client_focused) {
    client_t *t = wm.client_stack_list;
    for (; t && !client_is_visible(t); t = t->stack_next);
    wm.client_focused = t;
  }
}

task_in_tag_t *client_get_task_in_tag(client_t *client, tag_t *tag) {
  for (task_in_tag_t *task = tag->task_list; task; task = task->next) {
    if (task->client == client) return task;
  }
  return nullptr;
}

static void client_add_to_tag(client_t *client, tag_t *tag) {
  if (client_get_task_in_tag(client, tag)) return;

  task_in_tag_t *task = p_new(task_in_tag_t, 1);
  task->client = client;
  task->next = tag->task_list;
  tag->task_list = task;
}

static void client_remove_from_tag(client_t *client, tag_t *tag) {
  for (task_in_tag_t **task = &tag->task_list; *task; task = &(*task)->next) {
    if ((*task)->client != client) continue;
    p_delete(task);
    *task = (*task)->next;
  }
}

static inline void client_wipe(client_t *client) {
  p_delete(&client->class);
  p_delete(&client->instance);
  p_delete(&client->name);
  p_delete(&client->icon_name);
  p_delete(&client->net_name);
  p_delete(&client->net_icon_name);
  p_delete(&client->role);
  p_delete(&client);
}

static inline void client_tags_apply(client_t *client) {
  for (tag_t *tag = client->monitor->tag_list; tag; tag = tag->next) {
    if (client->tags & tag->mask) {
      client_add_to_tag(client, tag);
    } else {
      client_remove_from_tag(client, tag);
    }
  }

  if (client->tags == 0) client_wipe(client);
}

static inline void client_update_names(client_t *c) {
  xwindow_get_text_property(c->window, WM_NAME, &c->name);
  xwindow_get_text_property(c->window, WM_ICON_NAME, &c->icon_name);
  xwindow_get_text_property(c->window, _NET_WM_NAME, &c->net_name);
  xwindow_get_text_property(c->window, _NET_WM_ICON_NAME, &c->net_icon_name);
}

void client_manage(xcb_window_t window,
                   xcb_get_geometry_reply_t *geometry_reply) {
  client_t *c = p_new(client_t, 1);
  c->window = window;

  client_attach_list(c);
  client_attach_stack(c);

  {
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie =
      xcb_icccm_get_wm_class_unchecked(wm.xcb_conn, c->window);
    if (xcb_icccm_get_wm_class_reply(wm.xcb_conn, cookie, &prop, nullptr)) {
      c->class = strdup(prop.class_name);
      c->instance = strdup(prop.instance_name);
      xcb_icccm_get_wm_class_reply_wipe(&prop);
    }
    xwindow_get_text_property(c->window, WM_WINDOW_ROLE, &c->role);
  }

  {
    xcb_get_property_cookie_t cookie =
      xcb_icccm_get_wm_transient_for_unchecked(wm.xcb_conn, window);
    xcb_icccm_get_wm_transient_for_reply(wm.xcb_conn, cookie,
                                         &c->transient_for_window, nullptr);

    client_t *transient_for_client = nullptr;
    if (c->transient_for_window && (transient_for_client = client_get_by_window(
                                      c->transient_for_window))) {
      c->monitor = transient_for_client->monitor;
      c->tags = transient_for_client->tags;
    } else {
      c->monitor = wm.current_monitor;
      c->tags = c->monitor->selected_tag->mask;
    }

    client_tags_apply(c);
  }

  monitor_arrange(c->monitor);
  {
    const xcb_params_cw_t params = {
      .event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                    XCB_EVENT_MASK_PROPERTY_CHANGE |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
    };
    xcb_aux_change_window_attributes(wm.xcb_conn, window, XCB_CW_EVENT_MASK,
                                     &params);
    xcb_map_window(wm.xcb_conn, window);
  }

  client_update_names(c);
  monitor_draw_bar(c->monitor);
  xcb_flush(wm.xcb_conn);
}

char **client_get_task_title(client_t *client) {
  if (client->net_icon_name) return &client->net_icon_name;
  if (client->net_name) return &client->net_name;
  if (client->icon_name) return &client->icon_name;
  if (client->name) return &client->name;
  return nullptr;
}

bool client_need_layout(client_t *client) {
  if (client->floating || client->fullscreen || client->maximize ||
      client->minimize) {
    return false;
  }
  return true;
}

void client_move_to(client_t *client, int16_t x, int16_t y) {
  client->geometry.x = x;
  client->geometry.y = y;
  uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;
  const xcb_params_configure_window_t params = {.x = x, .y = y};
  xcb_aux_configure_window(wm.xcb_conn, client->window, mask, &params);
  logger("== client 0x%x move to x: %d, y: %d\n", client->window, x, y);
}

void client_resize(client_t *client, uint16_t width, uint16_t height) {
  client->geometry.width = width;
  client->geometry.height = height;
  uint16_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
  const xcb_params_configure_window_t params = {
    .width = width,
    .height = height,
  };
  xcb_aux_configure_window(wm.xcb_conn, client->window, mask, &params);
  logger("== client 0x%x resize to width: %d, height: %d\n", client->window,
         width, height);
}

bool client_is_visible(client_t *client) {
  return client->tags & client->monitor->selected_tag->mask;
}
