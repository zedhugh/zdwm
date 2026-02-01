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

client_t *client_get_next_by_class(client_t *current, const char *class) {
  client_t *start = current ? current->next : wm.client_list;
  for (client_t *c = start; c; c = c->next) {
    if (strcmp(class, c->class) == 0) return c;
  }

  for (client_t *c = wm.client_list; c && c != start; c = c->next) {
    if (strcmp(class, c->class) == 0) return c;
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

  xwindow_set_wm_desktop(client->window, tag->index);

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
  p_delete(&client->net_name);
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
  xwindow_get_text_property(c->window, _NET_WM_NAME, &c->net_name);
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

static void client_init_tag_by_wm_desktop(client_t *client) {
  uint32_t tag_index = 0;
  if (!xwindow_get_wm_desktop(client->window, &tag_index)) return;
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    for (tag_t *tag = m->tag_list; tag; tag = tag->next) {
      if (tag->index == tag_index) {
        client->monitor = m;
        client->tags = tag->mask;
        return;
      }
    }
  }
}

void client_manage(xcb_window_t window,
                   xcb_get_geometry_reply_t *geometry_reply) {
  client_t *c = p_new(client_t, 1);
  c->window = window;
  c->old_geometry.x = geometry_reply->x;
  c->old_geometry.y = geometry_reply->y;
  c->old_geometry.width = geometry_reply->width;
  c->old_geometry.height = geometry_reply->height;
  c->geometry = c->old_geometry;
  c->old_border_width = geometry_reply->border_width;

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
    client_init_tag_by_wm_desktop(c);
    client_tags_apply(c);
  }

  client_attach_list(c);
  client_attach_stack(c);
  client_update_window_type(c);
  client_update_size_hints(c);

  xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_APPEND, wm.screen->root,
                      _NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, 1, &window);

  monitor_arrange(c->monitor);
  monitor_draw_bar(c->monitor);

  wm_restack_clients();
  if (c->tags & c->monitor->selected_tag->mask) client_focus(c);

  xcb_flush(wm.xcb_conn);
}

char **client_get_task_title(client_t *client) {
  if (client->net_name) return &client->net_name;
  if (client->name) return &client->name;
  return nullptr;
}

bool client_need_layout(client_t *client) {
  if (client->floating || client->fullscreen || client->maximize ||
      client->minimize || client->size_freeze) {
    return false;
  }
  return true;
}

void client_send_to_tag(client_t *client, uint32_t tag_mask) {
  client->tags = tag_mask;
  client_tags_apply(client);
  monitor_select_tag(client->monitor, tag_mask);
}

void client_send_to_monitor(client_t *client, monitor_t *monitor) {
  if (client->monitor == monitor) return;

  for (tag_t *tag = client->monitor->tag_list; tag; tag = tag->next) {
    client_remove_from_tag(client, tag);
  }

  monitor_t *m = client->monitor;

  client->monitor = monitor;
  client->tags = monitor->selected_tag->mask;
  client_add_to_tag(client, monitor->selected_tag);

  wm.current_monitor = monitor;
  monitor_arrange(m);
  monitor_arrange(monitor);
  monitor_draw_bar(m);
  monitor_draw_bar(monitor);

  xcb_flush(wm.xcb_conn);
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

void client_update_window_type(client_t *client) {
  xcb_window_t window = client->window;
  xcb_get_property_cookie_t state_cookie = xcb_get_property_unchecked(
    wm.xcb_conn, false, window, _NET_WM_STATE, XCB_ATOM_ATOM, 0, 1);
  xcb_get_property_cookie_t window_type_cookie = xcb_get_property_unchecked(
    wm.xcb_conn, false, window, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 0, 1);
  xcb_get_property_reply_t *state_reply =
    xcb_get_property_reply(wm.xcb_conn, state_cookie, nullptr);
  xcb_get_property_reply_t *window_type_reply =
    xcb_get_property_reply(wm.xcb_conn, window_type_cookie, nullptr);

  if (state_reply) {
    xcb_atom_t state = *(xcb_atom_t *)xcb_get_property_value(state_reply);
    if (state == _NET_WM_STATE_FULLSCREEN) {
      client_set_fullscreen(client, true);
    }
    p_delete(&state_reply);
  }

  if (window_type_reply) {
    xcb_atom_t window_type =
      *(xcb_atom_t *)xcb_get_property_value(window_type_reply);
    if (window_type == _NET_WM_WINDOW_TYPE_DIALOG) {
      client_set_floating(client, true);
    }
    p_delete(&window_type_reply);
  }
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

void client_update_size_hints(client_t *client) {
  xcb_connection_t *conn = wm.xcb_conn;
  xcb_size_hints_t hints;
  xcb_get_property_cookie_t cookie =
    xcb_icccm_get_wm_normal_hints_unchecked(conn, client->window);
  if (!xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, nullptr)) {
    return;
  }

  int32_t min_width = 0, min_height = 0, max_width = 0, max_height = 0;
  if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE) {
    min_width = hints.min_width;
    min_height = hints.min_height;
  }
  if (hints.flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE) {
    max_width = hints.max_width;
    max_height = hints.max_height;
  }
  if (min_width == max_width && min_height == max_height) {
    client->size_freeze = true;
    client_set_floating(client, true);
  }
}

void client_set_floating(client_t *client, bool floating) {
  if (client->floating == floating || client->fullscreen) return;

  logger("== client set floating: %s\n", floating ? "true" : "false");
  logger("== old geometry: x -> %d, y -> %d, width -> %u, height -> %u\n",
         client->old_geometry.x, client->old_geometry.y,
         client->old_geometry.width, client->old_geometry.height);
  client->floating = floating;
  if (floating) {
    client_apply_workarea_geometry(client, client->old_geometry);
    client->old_geometry = client->geometry;
  }

  monitor_arrange(client->monitor);
}

void client_set_fullscreen(client_t *client, bool fullscreen) {
  if (client->fullscreen == fullscreen) return;

  client->fullscreen = fullscreen;
  if (fullscreen) {
    xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, client->window,
                        _NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
                        &_NET_WM_STATE_FULLSCREEN);
    client->old_border_width = client->border_width;
    if (client->floating) client->old_geometry = client->geometry;
    client_change_border_width(client, 0);
    client_apply_geometry(client, client->monitor->geometry);
    client_stack_raise(client);
  } else {
    xcb_change_property(wm.xcb_conn, XCB_PROP_MODE_REPLACE, client->window,
                        _NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, 0);
    client_change_border_width(client, client->old_border_width);
    client_apply_geometry(client, client->old_geometry);
  }
  monitor_arrange(client->monitor);
  client_focus(client);
}

static void update_client_list(void) {
  xcb_connection_t *conn = wm.xcb_conn;
  xcb_window_t root = wm.screen->root;
  xcb_atom_t atom = _NET_CLIENT_LIST;
  uint8_t mode = XCB_PROP_MODE_PREPEND;
  xcb_atom_t type = XCB_ATOM_WINDOW;

  xcb_delete_property(conn, root, atom);
  for (client_t *c = wm.client_list; c; c = c->next) {
    xcb_change_property(conn, mode, root, atom, type, 32, 1, &c->window);
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

  update_client_list();

  wm_restack_clients();
  monitor_deal_focus(m);
  monitor_arrange(m);
  monitor_draw_bar(m);
  xcb_flush(wm.xcb_conn);
}

void client_apply_geometry(client_t *client, area_t geometry) {
  if (client->geometry.x != geometry.x || client->geometry.y != geometry.y) {
    client_move_to(client, geometry.x, geometry.y);
  }
  if (client->geometry.width != geometry.width ||
      client->geometry.height != geometry.height) {
    client_resize(client, geometry.width, geometry.height);
  }
}

/**
 * @brief 调整 client 的几何信息但需让 client 任在其 monitor 的 workarea 中
 * @param client 要调整几何信息的 client
 * @param geometry 目标几何信息
 */
void client_apply_workarea_geometry(client_t *client, area_t geometry) {
  area_t workarea = client->monitor->workarea;
  uint16_t width = MIN(geometry.width, workarea.width);
  uint16_t height = MIN(geometry.height, workarea.height);
  int16_t x = MAX(geometry.x, workarea.x);
  int16_t y = MAX(geometry.y, workarea.y);
  if (x + width + client->border_width * 2 > workarea.x + workarea.width) {
    x = workarea.x + workarea.width - width - client->border_width * 2;
  }
  if (y + height + client->border_width * 2 > workarea.y + workarea.height) {
    y = workarea.y + workarea.width - height - client->border_width * 2;
  }

  area_t rect = {.x = x, .y = y, .width = width, .height = height};
  client_apply_geometry(client, rect);
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
