#include "interface/tray.h"

#include <stddef.h>
#include <stdint.h>
#include <zdwm/types.h>

#include "interface/types.h"
#include "internal.h"

static window_id_t tray_host_window(void *handle) {
  backend_t *backend = handle;
  if (backend == nullptr || !backend->tray.enabled) {
    return ZDWM_WINDOW_ID_INVALID;
  }

  return backend->tray.host_window;
}

static size_t tray_icon_count(void *handle) {
  backend_t *backend = handle;
  if (backend == nullptr || !backend->tray.enabled) return 0;

  return backend->tray.icon_count;
}

static int32_t tray_icon_size(void *handle) {
  backend_t *backend = handle;
  if (backend == nullptr || !backend->tray.enabled) return 0;

  return backend->tray.icon_size;
}

static void tray_place(void *handle, int32_t x) {
  backend_t *backend = handle;
  if (backend == nullptr || !backend->tray.enabled) return;

  auto conn = backend->conn;
  auto tray = &backend->tray;
  uint16_t value_mask = XCB_CONFIG_WINDOW_X;
  xcb_configure_window_value_list_t value_list = {.x = x};
  xcb_configure_window_aux(conn, tray->container, value_mask, &value_list);
}

static void
tray_set_listeners(void *handle, tray_icons_change_cb_t cb, void *user_data) {
  backend_t *backend = handle;
  if (backend == nullptr) return;

  backend->tray.icon_change_cb = cb;
  backend->tray.cb_user_data = user_data;
}

const tray_api_t tray_api = {
  .host_window = tray_host_window,
  .icon_count = tray_icon_count,
  .icon_size = tray_icon_size,
  .place = tray_place,
  .set_listener = tray_set_listeners,
};
