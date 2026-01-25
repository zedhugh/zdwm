#include "event.h"

#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon.h>

#include "action.h"
#include "atoms-extern.h"
#include "client.h"
#include "default_config.h"
#include "monitor.h"
#include "types.h"
#include "utils.h"
#include "wm.h"
#include "xkb.h"
#include "xwindow.h"

static void button_press(xcb_button_press_event_t *ev) {
  click_area_t click_area = click_none;
  user_action_arg_t arg = {0};

  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    if (ev->event == m->bar_window) {
      if (wm.current_monitor != m) wm.current_monitor = m;

      if (ev->event_x >= m->tag_extent.start &&
          ev->event_x <= m->tag_extent.end) {
        click_area = click_tag;

        for (tag_t *tag = m->tag_list; tag; tag = tag->next) {
          if (ev->event_x >= tag->bar_extent.start &&
              ev->event_x <= tag->bar_extent.end) {
            arg.ui = tag->mask;
            break;
          }
        }
      }
      break;
    }
  }

  for (int i = 0; i < countof(button_list); i++) {
    if (click_area == button_list[i].click_area &&
        button_list[i].button == ev->detail &&
        button_list[i].modifiers == ev->state) {
      button_list[i].func(&arg);
    }
  }
}

static void configure_request(xcb_configure_request_event_t *ev) {
  client_t *client = client_get_by_window(ev->window);
  if (client) {
    if (ev->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
      client_change_border_width(client, ev->border_width);
    } else {
      tag_t *tag = client->monitor->selected_tag;
      task_in_tag_t *task = client_get_task_in_tag(client, tag);
      if (!task || !(client->floating || !tag->layout->arrange)) return;

      if (ev->value_mask & XCB_CONFIG_WINDOW_X) task->geometry.x = ev->x;
      if (ev->value_mask & XCB_CONFIG_WINDOW_Y) task->geometry.y = ev->y;
      if (ev->value_mask & XCB_CONFIG_WINDOW_WIDTH)
        task->geometry.width = ev->width;
      if (ev->value_mask & XCB_CONFIG_WINDOW_HEIGHT)
        task->geometry.height = ev->height;
      client_apply_task_geometry(client, task);
    }
  } else {
    uint16_t value_mask = ev->value_mask;
    xcb_configure_window_value_list_t value_list = {
      .x = ev->x,
      .y = ev->y,
      .width = ev->width,
      .height = ev->height,
      .border_width = ev->border_width,
      .sibling = ev->sibling,
      .stack_mode = ev->stack_mode,
    };
    xcb_configure_window_aux(wm.xcb_conn, ev->window, value_mask, &value_list);
    xcb_flush(wm.xcb_conn);
  }
}

static void key_press(xcb_key_press_event_t *ev) {
  bool is_press = XCB_EVENT_RESPONSE_TYPE(ev) == XCB_KEY_PRESS;
  xcb_keysym_t keysym = xcb_key_press_lookup_keysym(wm.key_symbols, ev, 0);
  char key_name[16];
  if (xkb_keysym_get_name(keysym, key_name, sizeof(key_name)) != -1) {
    logger("key %s: %s\n", is_press ? "press" : "release", key_name);
  }

  if (!is_press) return;

  for (int i = 0; i < countof(key_list); i++) {
    keyboard_t key = key_list[i];
    if (keysym == key.keysym && ev->state == key.modifiers && key.func) {
      key.func(&key.arg);
      return;
    }
  }
}

static void map_request(xcb_map_request_event_t *ev) {
  xcb_get_window_attributes_cookie_t wa_cookie =
    xcb_get_window_attributes_unchecked(wm.xcb_conn, ev->window);
  xcb_get_window_attributes_reply_t *wa_reply =
    xcb_get_window_attributes_reply(wm.xcb_conn, wa_cookie, nullptr);

  if (!wa_reply) return;
  if (wa_reply->override_redirect) {
    p_delete(&wa_reply);
    return;
  }

  client_t *c = client_get_by_window(ev->window);
  if (!c) {
    xcb_get_geometry_cookie_t geo_cookie =
      xcb_get_geometry(wm.xcb_conn, ev->window);
    xcb_get_geometry_reply_t *geo_reply =
      xcb_get_geometry_reply(wm.xcb_conn, geo_cookie, nullptr);
    if (!geo_reply) {
      p_delete(&wa_reply);
      return;
    }

    client_manage(ev->window, geo_reply);
    p_delete(&geo_reply);
  }

  p_delete(&wa_reply);
}

static void client_message(xcb_client_message_event_t *ev) {
  client_t *c = client_get_by_window(ev->window);
  if (c == nullptr) return;

  if (ev->type == _NET_ACTIVE_WINDOW) {
    for (tag_t *t = c->monitor->tag_list; t; t = t->next) {
      if ((t->mask & c->tags) == 0) continue;

      wm.current_monitor = c->monitor;
      monitor_select_tag(c->monitor, t->mask);
      client_focus(c);
      return;
    }
  }
}

static void property_notify(xcb_property_notify_event_t *ev) {
  client_t *client = client_get_by_window(ev->window);
  if (client == nullptr) return;

  if (ev->atom == WM_NAME) {
    xwindow_get_text_property(ev->window, ev->atom, &client->name);
    monitor_draw_bar(client->monitor);
  } else if (ev->atom == WM_ICON_NAME) {
    if (client->icon_name) p_delete(&client->icon_name);
    xwindow_get_text_property(ev->window, ev->atom, &client->icon_name);
    monitor_draw_bar(client->monitor);
  } else if (ev->atom == _NET_WM_NAME) {
    xwindow_get_text_property(ev->window, ev->atom, &client->net_name);
    monitor_draw_bar(client->monitor);
  } else if (ev->atom == _NET_WM_ICON_NAME) {
    xwindow_get_text_property(ev->window, ev->atom, &client->net_icon_name);
    monitor_draw_bar(client->monitor);
  } else if (ev->atom == XCB_ATOM_WM_HINTS) {
    client_update_wm_hints(client);
    monitor_draw_bar(client->monitor);
  }
}

static void destroy_notify(xcb_destroy_notify_event_t *ev) {
  client_t *client = client_get_by_window(ev->window);
  if (client) client_unmanage(client);
}

static void unmap_notify(xcb_unmap_notify_event_t *ev) {
  client_t *client = client_get_by_window(ev->window);
  if (client) client_unmanage(client);
}

static void map_notify(xcb_map_notify_event_t *ev) {
  /* 锁屏重新进入桌面后绘制 bar 内容以免 bar 不显示内容 */
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    if (ev->window == m->bar_window) {
      monitor_draw_bar(m);
      xcb_flush(wm.xcb_conn);
      return;
    }
  }
}

static void expose(xcb_expose_event_t *ev) {
  if (ev->count > 0) return;

  monitor_t *monitor = wm_get_monitor_by_window(ev->window);
  monitor_draw_bar(monitor);
  xcb_flush(wm.xcb_conn);
}

static void handle_xcb_event(xcb_generic_event_t *event) {
  uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);
  const char *label = xcb_event_get_label(event_type);
  logger("event type: %u[%s], xkb_event: %u\n", event_type, label,
         wm.event_base_xkb);

  switch (event_type) {
#define EVENT(type, callback) \
  case type:                  \
    callback((void *)event);  \
    return

    EVENT(XCB_BUTTON_PRESS, button_press);
    EVENT(XCB_KEY_PRESS, key_press);
    EVENT(XCB_KEY_RELEASE, key_press);
    EVENT(XCB_CONFIGURE_REQUEST, configure_request);
    EVENT(XCB_MAP_REQUEST, map_request);
    EVENT(XCB_MAP_NOTIFY, map_notify);
    EVENT(XCB_EXPOSE, expose);
    EVENT(XCB_CLIENT_MESSAGE, client_message);
    EVENT(XCB_PROPERTY_NOTIFY, property_notify);
    EVENT(XCB_UNMAP_NOTIFY, unmap_notify);
    EVENT(XCB_DESTROY_NOTIFY, destroy_notify);

#undef EVENT
  }

  /* 处理 XKB 事件 */
  if (wm.event_base_xkb != 0 && event_type == wm.event_base_xkb) {
    xkb_handle_event(event);
  }
}

static gboolean xcb_event_loop(GIOChannel *channel, GIOCondition condition,
                               gpointer user_data) {
  if (condition & (G_IO_HUP | G_IO_ERR)) {
    wm_quit();
    return false;
  }

  xcb_generic_event_t *event = nullptr;
  while ((event = xcb_poll_for_event(wm.xcb_conn))) {
    handle_xcb_event(event);
    p_delete(&event);
  }

  return true;
}

void setup_event_loop(void) {
  int fd = xcb_get_file_descriptor(wm.xcb_conn);
  GIOChannel *channel = g_io_channel_unix_new(fd);
  g_io_channel_set_encoding(channel, nullptr, nullptr);
  GIOCondition cond = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_io_add_watch(channel, cond, xcb_event_loop, nullptr);
  g_io_channel_unref(channel);
}
