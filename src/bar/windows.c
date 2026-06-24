#include "bar/windows.h"

#include <assert.h>
#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/listeners.h>
#include <zdwm/types.h>

#include "base/array.h"
#include "base/memory.h"
#include "common/listeners.h"

typedef struct bar_windows_state_t {
  zdwm_output_id_t output_id;
  zdwm_workspace_id_t workspace_id;

  zdwm_window_t *windows;
  size_t count;
  size_t capacity;

  bar_windows_config_t config;

  bool workspace_inited;
  bool dirty;
} bar_windows_state_t;

static zdwm_window_t *
state_get_window(bar_windows_state_t *state, zdwm_window_id_t window_id) {
  for (size_t i = 0; i < state->count; ++i) {
    auto window = &state->windows[i];
    if (window->id == window_id) return window;
  }

  return nullptr;
}

static void *
bar_windows_create_state(zdwm_output_id_t output_id, void *config) {
  const bar_windows_config_t *windows_config = config;
  assert(windows_config);

  auto state = p_new(bar_windows_state_t, 1);
  state->output_id = output_id;
  state->workspace_id = ZDWM_WORKSPACE_ID_INVALID;
  state->config = *windows_config;

  return state;
}

static void bar_windows_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cells,
  void *state
) {
  bar_windows_state_t *data = state;
  if (!data->workspace_inited || !data->dirty) return;

  zdwm_window_t *windows = nullptr;
  size_t count = 0, capacity = 0;
  for (size_t i = 0; i < data->count; ++i) {
    auto win = &data->windows[i];
    if (win->workspace != data->workspace_id || win->skip_taskbar) continue;

    auto window = array_push(windows, count, capacity);
    *window = *win;
  }

  cells->set_cell_count(item, count);
  for (size_t i = 0; i < count; ++i) {
    auto window = &windows[i];
    auto bg = window->focused ? data->config.focused_bg : data->config.bg;
    auto fg = window->focused ? data->config.focused_fg : data->config.fg;
    cells->cell_set_text(item, i, window->title);
    cells->cell_set_bg(item, i, bg);
    cells->cell_set_fg(item, i, fg);
  }
  p_delete(&windows);

  data->dirty = false;
}

static void bar_windows_destroy_state(void *state) {
  bar_windows_state_t *data = state;

  p_delete(&data->windows);
  p_delete(&data);
}

zdwm_bar_item_type_t bar_windows = {
  .create_state = bar_windows_create_state,
  .update = bar_windows_update,
  .destroy_state = bar_windows_destroy_state,
  .update_interval_ms = 0,
};

static void bar_windows_workspace_active(
  zdwm_output_id_t output_id,
  zdwm_workspace_id_t workspace_id,
  void *user_data
) {
  bar_windows_state_t *state = user_data;
  if (state->output_id != output_id) return;
  if (state->workspace_id == workspace_id) return;

  state->workspace_id = workspace_id;

  state->workspace_inited = true;
  state->dirty = true;
}

static void
bar_windows_add_window(const zdwm_window_t *window, void *user_data) {
  bar_windows_state_t *state = user_data;
  auto win = array_push(state->windows, state->count, state->capacity);
  *win = *window;

  state->dirty = true;
}

static void
bar_windows_initial(const zdwm_window_t *list, size_t count, void *user_data) {
  bar_windows_state_t *state = user_data;

  for (size_t i = 0; i < count; ++i) {
    auto window = &list[i];
    bar_windows_add_window(window, state);
  }

  state->dirty = true;
}

static void
bar_windows_update_window(const zdwm_window_t *window, void *user_data) {
  bar_windows_state_t *state = user_data;

  auto old_window = state_get_window(state, window->id);
  *old_window = *window;

  state->dirty = true;
}

static void
bar_windows_remove_window(zdwm_window_id_t window_id, void *user_data) {
  bar_windows_state_t *state = user_data;

  bool found = false;
  size_t index = 0;

  for (size_t i = 0; i < state->count; ++i) {
    auto window = &state->windows[i];
    if (window->id == window_id) {
      found = true;
      index = i;
      break;
    }
  }

  if (!found) return;

  if (array_erase(state->windows, state->count, index)) {
    state->dirty = true;
  }
}

void bar_windows_add_listeners(listeners_t *listeners, void *state) {
#define ADD(ADD_FN, LISTENER) ADD_FN(listeners, LISTENER, state)

  ADD(listeners_add_active_workspace_listener, bar_windows_workspace_active);
  ADD(listeners_add_initial_window_listener, bar_windows_initial);
  ADD(listeners_add_window_added_listener, bar_windows_add_window);
  ADD(listeners_add_window_updated_listener, bar_windows_update_window);
  ADD(listeners_add_window_removed_listener, bar_windows_remove_window);

#undef ADD
}
