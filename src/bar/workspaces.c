#include "bar/workspaces.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/listeners.h>
#include <zdwm/types.h>

#include "base/array.h"
#include "base/memory.h"

static size_t state_workspace_get_urgent_window_count(
  const bar_workspace_state_t *state,
  zdwm_workspace_id_t workspace_id
) {
  size_t count = 0;

  for (size_t i = 0; i < state->window_count; ++i) {
    auto window = &state->windows[i];
    if (window->workspace == workspace_id && window->urgent) {
      count++;
    }
  }

  return count;
}

static size_t state_workspace_get_window_count(
  const bar_workspace_state_t *state,
  zdwm_workspace_id_t workspace_id
) {
  size_t count = 0;

  for (size_t i = 0; i < state->window_count; ++i) {
    auto window = &state->windows[i];
    if (window->workspace == workspace_id && !window->skip_taskbar) {
      count++;
    }
  }

  return count;
}

static zdwm_window_t *state_get_window(
  const bar_workspace_state_t *state,
  zdwm_window_id_t window_id
) {
  for (size_t i = 0; i < state->window_count; ++i) {
    auto window = &state->windows[i];
    if (window->id == window_id) return window;
  }

  return nullptr;
}

void *bar_workspaces_create_state(zdwm_output_id_t output_id, void *config) {
  const bar_workspace_palette_t *palette = config;
  assert(palette);

  auto state       = p_new(bar_workspace_state_t, 1);
  state->output_id = output_id;
  state->palette   = *palette;

  return state;
}

void bar_workspaces_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cell_api,
  void *state
) {
  auto data = (bar_workspace_state_t *)state;
  if (!data->list_inited || !data->active_inited || !data->dirty) return;

  if (data->count != cell_api->get_cell_count(item)) {
    cell_api->set_cell_count(item, data->count);
  }

  auto palette = &data->palette;
  for (size_t i = 0; i < data->count; ++i) {
    auto workspace = &data->workspaces[i];
    auto info      = &workspace->info;

    workspace->window_count = state_workspace_get_window_count(state, info->id);
    workspace->urgent_window_count =
      state_workspace_get_urgent_window_count(state, info->id);

    bool show_indicator = workspace->window_count;
    cell_api->cell_set_indicator(item, i, show_indicator);

    cell_api->cell_set_text(item, i, info->name);
    if (info->id == data->workspace_id) {
      cell_api->cell_set_bg(item, i, palette->active_bg);
      cell_api->cell_set_fg(item, i, palette->active_fg);
      continue;
    }

    if (workspace->urgent_window_count > 0) {
      cell_api->cell_set_bg(item, i, palette->urgent_bg);
      cell_api->cell_set_fg(item, i, palette->urgent_fg);
      continue;
    }

    cell_api->cell_set_bg(item, i, palette->bg);
    cell_api->cell_set_fg(item, i, palette->fg);
  }

  data->dirty = false;
}

zdwm_action_t bar_workspace_on_click(
  zdwm_bar_item_t *item,
  size_t cell_index,
  int32_t x,
  void *state
) {
  auto data      = (bar_workspace_state_t *)state;
  auto workspace = &data->workspaces[cell_index];

  if (workspace->info.id == data->workspace_id) {
    return (zdwm_action_t){.type = ZDWM_ACTION_NONE};
  }

  return (zdwm_action_t){
    .type                          = ZDWM_ACTION_WORKSPACE_SWITCH,
    .as.switch_workspace.workspace = workspace->info.id,
  };
}

void bar_workspace_destroy_state(void *state) {
  auto data = (bar_workspace_state_t *)state;
  p_delete(&data->workspaces);
  p_clear(data, 1);
}

void bar_workspace_list_filter(
  const zdwm_workspace_t *list,
  size_t count,
  void *user_data
) {
  auto state = (bar_workspace_state_t *)user_data;

  for (size_t i = 0; i < count; ++i) {
    auto workspace = &list[i];
    if (workspace->output_id != state->output_id) continue;

    auto item = array_push(state->workspaces, state->count, state->capacity);
    p_clear(item, 1);
    item->info = *workspace;
  }

  state->list_inited = true;
  state->dirty       = true;
}

void bar_workspace_active_updated(
  zdwm_output_id_t output_id,
  zdwm_workspace_id_t workspace_id,
  void *user_data
) {
  auto state = (bar_workspace_state_t *)user_data;
  if (state->output_id != output_id) return;
  if (state->workspace_id == workspace_id) return;

  state->workspace_id = workspace_id;

  state->active_inited = true;
  state->dirty         = true;
}

void bar_workspace_initial_windows(
  const zdwm_window_t *list,
  size_t count,
  void *user_data
) {
  for (size_t i = 0; i < count; ++i) {
    auto window = &list[i];
    bar_workspace_window_added(window, user_data);
  }
}

void bar_workspace_window_added(const zdwm_window_t *window, void *user_data) {
  auto state = (bar_workspace_state_t *)user_data;

  auto win =
    array_push(state->windows, state->window_count, state->window_capacity);
  *win = *window;

  state->dirty = true;
}

void bar_workspace_window_updated(
  const zdwm_window_t *window,
  void *user_data
) {
  auto state = (bar_workspace_state_t *)user_data;

  auto old_window = state_get_window(state, window->id);
  *old_window     = *window;
  state->dirty    = true;
}

void bar_workspace_window_removed(zdwm_window_id_t window_id, void *user_data) {
  auto state = (bar_workspace_state_t *)user_data;

  bool found   = false;
  size_t index = 0;

  for (size_t i = 0; i < state->window_count; ++i) {
    auto window = &state->windows[i];
    if (window->id == window_id) {
      found = true;
      index = i;
    }
  }

  if (!found) return;

  if (array_erase(state->windows, state->window_count, index)) {
    state->dirty = true;
  }
}
