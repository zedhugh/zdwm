#include "client.h"

#include <stdint.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "atoms-extern.h"
#include "base.h"
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

task_in_tag_t *client_get_next_task_in_tag(client_t *client, tag_t *tag) {
  if (!tag->task_list) return nullptr;

  task_in_tag_t *task = tag->task_list;
  for (; task && task->client != client; task = task->next);
  return task && task->next ? task->next : tag->task_list;
}

task_in_tag_t *client_get_previous_task_in_tag(client_t *client, tag_t *tag) {
  if (!tag->task_list) return nullptr;

  task_in_tag_t *task = tag->task_list;
  for (; task && task->next && task->next->client != client; task = task->next);
  return task ? task : tag->task_list;
}

static void client_add_to_tag(client_t *client, tag_t *tag) {
  if (client_get_task_in_tag(client, tag)) return;

  task_in_tag_t *task = p_new(task_in_tag_t, 1);
  task->client = client;
  task->next = tag->task_list;
  tag->task_list = task;
  task->geometry = client->geometry;
}

static void client_remove_from_tag(client_t *client, tag_t *tag) {
  task_in_tag_t **task = &tag->task_list;
  while (*task) {
    if ((*task)->client == client) {
      task_in_tag_t *t = *task;
      *task = (*task)->next;
      p_delete(&t);
      continue;
    }

    task = &(*task)->next;
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

static inline void client_init_geometry(client_t *c) {
  area_t workarea = c->monitor->workarea;
  if (c->geometry.x + client_width(c) > workarea.x + workarea.width) {
    c->geometry.x = workarea.x + workarea.width - client_width(c);
  }
  if (c->geometry.y + client_height(c) > workarea.y + workarea.height) {
    c->geometry.y = workarea.y + workarea.height - client_height(c);
  }
  c->geometry.x = MAX(c->geometry.x, workarea.x);
  c->geometry.y = MAX(c->geometry.y, workarea.y);

  client_move_to(c, c->geometry.x, c->geometry.y);
  client_resize(c, c->geometry.width, c->geometry.height);
  client_change_border_color(c, &wm.color_set.active_border_color);
  client_change_border_width(c, wm.border_width);
}

void client_manage(xcb_window_t window,
                   xcb_get_geometry_reply_t *geometry_reply) {
  client_t *c = p_new(client_t, 1);
  c->window = window;
  c->geometry.x = geometry_reply->x;
  c->geometry.y = geometry_reply->y;
  c->geometry.width = geometry_reply->width;
  c->geometry.height = geometry_reply->height;
  c->border_width = geometry_reply->border_width;

  {
    const xcb_params_cw_t params = {
      .event_mask = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
                    XCB_EVENT_MASK_PROPERTY_CHANGE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
    };
    xcb_aux_change_window_attributes(wm.xcb_conn, window, XCB_CW_EVENT_MASK,
                                     &params);
    xcb_map_window(wm.xcb_conn, window);
  }
  client_update_names(c);
  xwindow_get_text_property(c->window, WM_WINDOW_ROLE, &c->role);
  client_update_wm_hints(c);

  {
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie =
      xcb_icccm_get_wm_class_unchecked(wm.xcb_conn, c->window);
    if (xcb_icccm_get_wm_class_reply(wm.xcb_conn, cookie, &prop, nullptr)) {
      c->class = strdup(prop.class_name);
      c->instance = strdup(prop.instance_name);
      xcb_icccm_get_wm_class_reply_wipe(&prop);
    }
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

    client_init_geometry(c);
    client_tags_apply(c);
  }

  client_attach_list(c);
  client_attach_stack(c);

  monitor_arrange(c->monitor);
  monitor_draw_bar(c->monitor);

  wm_restack_clients();
  if (c->tags & c->monitor->selected_tag->mask) client_focus(c);

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

void client_change_border_color(client_t *client, color_t *color) {
  uint16_t mask = XCB_CW_BORDER_PIXEL;
  const xcb_params_cw_t params = {.border_pixel = color->argb};
  xcb_aux_change_window_attributes(wm.xcb_conn, client->window, mask, &params);
  client->border_color = color;
  logger("== client 0x%x border color: RGBA(#%08x), ARGB(#%08x)\n",
         client->window, color->rgba, color->argb);
}

void client_change_border_width(client_t *client, uint16_t border_width) {
  if (client->border_width == border_width) return;
  uint16_t mask = XCB_CONFIG_WINDOW_BORDER_WIDTH;
  const xcb_params_configure_window_t params = {.border_width = border_width};
  xcb_aux_configure_window(wm.xcb_conn, client->window, mask, &params);
  client->border_width = border_width;
  logger("== client 0x%x border width: %u\n", client->window, border_width);
}

void client_update_wm_hints(client_t *client) {
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_hints(wm.xcb_conn, client->window);
  xcb_icccm_wm_hints_t hints;
  if (xcb_icccm_get_wm_hints_reply(wm.xcb_conn, cookie, &hints, nullptr)) {
    if (client == wm.client_focused &&
        (hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY)) {
      hints.flags &= ~XCB_ICCCM_WM_HINT_X_URGENCY;
      xcb_icccm_set_wm_hints(wm.xcb_conn, client->window, &hints);
    } else {
      client->urgent = hints.flags & XCB_ICCCM_WM_HINT_X_URGENCY;
    }
  }
}

void client_unmanage(client_t *client) {
  monitor_t *m = client->monitor;

  client_detach_list(client);
  client_detach_stack(client);

  for (tag_t *tag = m->tag_list; tag; tag = tag->next) {
    if (tag->mask & client->tags) client_remove_from_tag(client, tag);
  }

  client_wipe(client);

  wm_restack_clients();
  monitor_deal_focus(m);
  monitor_arrange(m);
  monitor_draw_bar(m);
  xcb_flush(wm.xcb_conn);
}

void client_apply_task_geometry(client_t *client, task_in_tag_t *task) {
  if (!client || !task || client != task->client) return;

  if (task->geometry.x != client->geometry.x ||
      task->geometry.y != client->geometry.y) {
    client_move_to(client, task->geometry.x, task->geometry.y);
  }
  if (task->geometry.width != client->geometry.width ||
      task->geometry.height != client->geometry.height) {
    client_resize(client, task->geometry.width, task->geometry.height);
  }
}

void client_focus(client_t *client) {
  wm.client_focused = client;
  xwindow_focus(client ? client->window : XCB_WINDOW_NONE);
}

void client_stack_raise(client_t *client) {
  client_detach_stack(client);
  client_attach_stack(client);
  wm_restack_clients();
}

bool client_is_visible(client_t *client) {
  return client->tags & client->monitor->selected_tag->mask;
}
