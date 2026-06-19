#include <stddef.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xproto.h>

#include "base/macros.h"
#include "base/memory.h"
#include "internal.h"

static const char *const cursor_name_map[] = {
  [ZDWM_CURSOR_NORMAL] = "left_ptr",
  [ZDWM_CURSOR_MOVE]   = "fleur",
  [ZDWM_CURSOR_RESIZE] = "bottom_right_corner",
};

static xcb_cursor_context_t *get_xcb_cursor_context(backend_t *backend) {
  if (backend->cursor_context) return backend->cursor_context;

  auto conn   = backend->conn;
  auto screen = backend->screen;
  if (xcb_cursor_context_new(conn, screen, &backend->cursor_context) == 0) {
    return backend->cursor_context;
  }

  return nullptr;
}

void cursor_init(backend_t *backend) {
  auto ctx = get_xcb_cursor_context(backend);
  if (!ctx) return;

  for (size_t i = 0; i < countof(backend->cursors); ++i) {
    auto cursor_name    = cursor_name_map[i];
    backend->cursors[i] = xcb_cursor_load_cursor(ctx, cursor_name);
  }
}

void cursor_cleanup(backend_t *backend) {
  constexpr size_t count = countof(backend->cursors);
  for (size_t i = 0; i < count; ++i) {
    xcb_free_cursor(backend->conn, backend->cursors[i]);
  }
  p_clear(backend->cursors, count);
  xcb_cursor_context_free(backend->cursor_context);
  backend->cursor_context = nullptr;
}

xcb_cursor_t cursor_get_xcb_cursor(backend_t *backend, cursor_t cursor) {
  if (backend->cursor_context == nullptr) return XCB_CURSOR_NONE;

  return backend->cursors[cursor];
}
