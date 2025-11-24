#include "xkb.h"

#include <assert.h>
#include <glib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xkb.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include "atoms-extern.h"
#include "default_config.h"
#include "utils.h"
#include "wm.h"
#include "xwindow.h"

static bool xkb_fill_rule_names_from_root(struct xkb_rule_names *xkb_names) {
  xcb_get_property_cookie_t cookie = xcb_get_property_unchecked(
    wm.xcb_conn, false, wm.screen->root, _XKB_RULES_NAMES,
    XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
  xcb_get_property_reply_t *reply =
    xcb_get_property_reply(wm.xcb_conn, cookie, nullptr);
  if (!reply) return false;

  if (reply->value_len == 0) {
    p_delete(&reply);
    return false;
  }

  const char *walk = xcb_get_property_value(reply);
  size_t remaining = xcb_get_property_value_length(reply);
  for (int i = 0; i < 5 && remaining > 0; i++) {
    size_t len = strnlen(walk, remaining);
    switch (i) {
      case 0:
        xkb_names->rules = strndup(walk, len);
        break;
      case 1:
        xkb_names->model = strndup(walk, len);
        break;
      case 2:
        xkb_names->layout = strndup(walk, len);
        break;
      case 3:
        xkb_names->variant = strndup(walk, len);
        break;
      case 4:
        xkb_names->options = strndup(walk, len);
        break;
    }
    remaining -= len + 1;
    walk = &walk[len + 1];
  }

  p_delete(&reply);
  return true;
}

static void xkb_fill_state(void) {
  int32_t device_id = -1;
  if (wm.have_xkb) {
    device_id = xkb_x11_get_core_keyboard_device_id(wm.xcb_conn);
    if (device_id == -1) warn("Failed get XKB device id");
  }

  if (device_id == -1) {
    struct xkb_rule_names names = {nullptr, nullptr, nullptr, nullptr, nullptr};
    if (!xkb_fill_rule_names_from_root(&names)) {
      warn(
        "Could not get _XKB_RULES_NAMES from root window, falling back to "
        "defaults.");
    }

    struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_names(
      wm.xkb_ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    wm.xkb_state = xkb_state_new(xkb_keymap);

    if (!wm.xkb_state) fatal("Failed create XKB state");

    xkb_keymap_unref(xkb_keymap);
    p_delete(&names.rules);
    p_delete(&names.model);
    p_delete(&names.layout);
    p_delete(&names.variant);
    p_delete(&names.options);
  } else {
    struct xkb_keymap *xkb_keymap = xkb_x11_keymap_new_from_device(
      wm.xkb_ctx, wm.xcb_conn, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!xkb_keymap) fatal("Failed get XKB keymap from device");
    wm.xkb_state =
      xkb_x11_state_new_from_device(xkb_keymap, wm.xcb_conn, device_id);
    if (!wm.xkb_state) fatal("Failed get XKB state from device");
    xkb_keymap_unref(xkb_keymap);
  }
}

static void xkb_init_keymap(void) {
  wm.xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!wm.xkb_ctx) fatal("Cannot get XKB context");

  xkb_fill_state();
}

#define DEVICE_SPEC XCB_XKB_ID_USE_CORE_KBD

void xkb_init(void) {
  wm.xkb_update_pending = false;
  wm.xkb_reload_keymap = false;

  wm.have_xkb = xkb_x11_setup_xkb_extension(
    wm.xcb_conn, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
    XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, &wm.event_base_xkb,
    nullptr);
  printf("xkb base event: %u\n", wm.event_base_xkb);
  if (!wm.have_xkb) {
    warn("XKB not found or not supported");
    xkb_init_keymap();
    return;
  }

  xcb_xkb_per_client_flags_cookie_t cookie = xcb_xkb_per_client_flags(
    wm.xcb_conn, DEVICE_SPEC, XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
    XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT, 0, 0, 0);
  xcb_discard_reply(wm.xcb_conn, cookie.sequence);
  uint16_t xkb_event = XCB_XKB_EVENT_TYPE_STATE_NOTIFY |
                       XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                       XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY;
  uint16_t map_parts =
    XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
    XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
    XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_KEY_BEHAVIORS |
    XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
  xcb_xkb_select_events(wm.xcb_conn, DEVICE_SPEC, xkb_event, 0, xkb_event,
                        map_parts, map_parts, nullptr);

  xkb_init_keymap();
}

void xkb_free(void) {
  if (wm.have_xkb) {
    xcb_xkb_select_events(wm.xcb_conn, DEVICE_SPEC, 0, 0, 0, 0, 0, nullptr);
  }

  xkb_state_unref(wm.xkb_state);
  xkb_context_unref(wm.xkb_ctx);
  wm.xkb_state = nullptr;
  wm.xkb_ctx = nullptr;
}

static void xkb_reload_keymap(void) {
  assert(wm.have_xkb);

  xcb_key_symbols_free(wm.key_symbols);
  wm.key_symbols = xcb_key_symbols_alloc(wm.xcb_conn);
  xwindow_grab_keys(wm.screen->root, key_list, countof(key_list));

  xkb_state_unref(wm.xkb_state);
  xkb_fill_state();
}

static gboolean xkb_refresh(gpointer ignore) {
  wm.xkb_update_pending = false;
  if (wm.xkb_reload_keymap) xkb_reload_keymap();

  wm.xkb_reload_keymap = false;

  return G_SOURCE_REMOVE;
}

static void xkb_schedule_refresh(void) {
  if (wm.xkb_update_pending) return;

  wm.xkb_update_pending = true;
  g_idle_add_full(G_PRIORITY_LOW, xkb_refresh, nullptr, nullptr);
}

void xkb_handle_event(xcb_generic_event_t *event) {
  assert(wm.have_xkb);

  printf("xkb event type: %u\n", event->pad0);

  /* XKB 中没有对应所有事件的泛型类型，xcb_generic_event_t 类型中的 pad0
   * 字段对应 XKB 事件中的 xkbType 字段，故用该字段判断 XKB 事件类型 */
  switch (event->pad0) {
    case XCB_XKB_NEW_KEYBOARD_NOTIFY: {
      wm.xkb_reload_keymap = true;
      xkb_schedule_refresh();
      break;
    }
    case XCB_XKB_MAP_NOTIFY: {
      wm.xkb_reload_keymap = true;
      xkb_schedule_refresh();
      break;
    }
    case XCB_XKB_STATE_NOTIFY: {
      xcb_xkb_state_notify_event_t *state_notify_event = (void *)event;
      xkb_state_update_mask(
        wm.xkb_state, state_notify_event->baseMods,
        state_notify_event->latchedMods, state_notify_event->lockedMods,
        state_notify_event->baseGroup, state_notify_event->latchedGroup,
        state_notify_event->lockedGroup);

      printf("changed: %u\n",
             state_notify_event->changed & XCB_XKB_STATE_PART_GROUP_STATE);
      if (state_notify_event->changed & XCB_XKB_STATE_PART_GROUP_STATE) {
        xkb_schedule_refresh();
      }
      break;
    }
  }
}
