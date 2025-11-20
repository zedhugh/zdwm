#include "event.h"

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xproto.h>

#include "monitor.h"
#include "types.h"
#include "utils.h"
#include "wm.h"

static void button_press(xcb_button_press_event_t *ev) {
  if (ev->event == wm.current_monitor->bar_window) {
    if (ev->event_x >= wm.current_monitor->tag_extent.start &&
        ev->event_x <= wm.current_monitor->tag_extent.end) {
      for (tag_t *tag = wm.current_monitor->tag_list; tag; tag = tag->next) {
        if (ev->event_x >= tag->bar_extent.start &&
            ev->event_x <= tag->bar_extent.end) {
          monitor_select_tag(wm.current_monitor, tag->mask);
          break;
        }
      }
    }
  }
}

static void handle_xcb_event(xcb_generic_event_t *event) {
  uint8_t event_type = XCB_EVENT_RESPONSE_TYPE(event);

  switch (event_type) {
#define EVENT(type, callback) \
  case type:                  \
    callback((void *)event);  \
    break

    EVENT(XCB_BUTTON_PRESS, button_press);

#undef EVENT
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
