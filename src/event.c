#include "event.h"

#include <glib.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "action.h"
#include "default_config.h"
#include "types.h"
#include "utils.h"
#include "wm.h"
#include "xkb.h"

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

static void key_press(xcb_key_press_event_t *ev) {
  xcb_keysym_t keysym = xcb_key_press_lookup_keysym(wm.key_symbols, ev, 0);
  for (int i = 0; i < countof(key_list); i++) {
    keyboard_t key = key_list[i];
    if (keysym == key.keysym && ev->state == key.modifiers && key.func) {
      key.func(&key.arg);
      return;
    }
  }
}

static void handle_xcb_event(xcb_generic_event_t *event) {
  uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);

  switch (event_type) {
#define EVENT(type, callback) \
  case type:                  \
    callback((void *)event);  \
    return

    EVENT(XCB_BUTTON_PRESS, button_press);
    EVENT(XCB_KEY_PRESS, key_press);

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
