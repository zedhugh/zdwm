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
#include "core/listeners.h"

typedef struct bar_workspace_t {
  zdwm_workspace_t info;
  zdwm_layout_notify_t layout;
  size_t window_count;
  size_t urgent_window_count;
} bar_workspace_t;

typedef struct bar_workspace_state_t {
  zdwm_output_id_t output_id;
  zdwm_workspace_id_t workspace_id;
  bar_workspace_t *workspaces;
  size_t count;
  size_t capacity;

  zdwm_window_t *windows;
  size_t window_count;
  size_t window_capacity;

  bar_workspace_config_t config;

  bool list_inited;
  bool active_inited;
  bool dirty;
} bar_workspace_state_t;

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

static void *
bar_workspaces_create_state(zdwm_output_id_t output_id, void *config) {
  const bar_workspace_config_t *workspace_config = config;
  assert(workspace_config);

  auto state          = p_new(bar_workspace_state_t, 1);
  state->output_id    = output_id;
  state->workspace_id = ZDWM_WORKSPACE_ID_INVALID;
  state->config       = *workspace_config;

  return state;
}

static void bar_workspaces_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cell_api,
  void *state
) {
  auto data = (bar_workspace_state_t *)state;
  if (!data->list_inited || !data->active_inited || !data->dirty) return;

  /* 当前 workspace 的 layout 指示器放在最后 */
  cell_api->set_cell_count(item, data->count + 1);
  auto layout_index = data->count;

  auto config = &data->config;
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
      cell_api->cell_set_bg(item, i, config->active_bg);
      cell_api->cell_set_fg(item, i, config->active_fg);

      cell_api->cell_set_bg(item, layout_index, config->layout_bg);
      cell_api->cell_set_fg(item, layout_index, config->layout_fg);
      cell_api->cell_set_text(item, layout_index, workspace->layout.symbol);

      continue;
    }

    if (workspace->urgent_window_count > 0) {
      cell_api->cell_set_bg(item, i, config->urgent_bg);
      cell_api->cell_set_fg(item, i, config->urgent_fg);
      continue;
    }

    cell_api->cell_set_bg(item, i, config->bg);
    cell_api->cell_set_fg(item, i, config->fg);
  }

  data->dirty = false;
}

static zdwm_action_t bar_workspace_on_click(zdwm_bar_click_params_t *params) {
  auto cell_index = params->cell_index;
  auto data       = (bar_workspace_state_t *)params->state;

  if (cell_index == data->count) {
    return (zdwm_action_t){
      .type            = ZDWM_ACTION_LAYOUT_CYCLE,
      .as.layout_cycle = {.delta = 1},
    };
  }

  auto workspace = &data->workspaces[cell_index];
  if (workspace->info.id == data->workspace_id) {
    return (zdwm_action_t){.type = ZDWM_ACTION_NONE};
  }

  return (zdwm_action_t){
    .type                          = ZDWM_ACTION_WORKSPACE_SWITCH,
    .as.switch_workspace.workspace = workspace->info.id,
  };
}

static void bar_workspace_destroy_state(void *state) {
  if (!state) return;

  auto data = (bar_workspace_state_t *)state;
  p_delete(&data->workspaces);
  p_delete(&data->windows);
  p_delete(&data);
}

zdwm_bar_item_type_t bar_workspace = {
  .create_state       = bar_workspaces_create_state,
  .update             = bar_workspaces_update,
  .on_click           = bar_workspace_on_click,
  .destroy_state      = bar_workspace_destroy_state,
  .update_interval_ms = 0,
};

typedef struct bar_workspace_listener_user_data_t {
  zdwm_bar_item_t *bar;
  bar_workspace_state_t *state;
} bar_workspace_listener_user_data_t;

static void bar_workspace_list_filter(
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

static void bar_workspace_active_updated(
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

static void
bar_workspace_layout_notify(zdwm_layout_notify_t layout, void *user_data) {
  auto state = (bar_workspace_state_t *)user_data;

  for (size_t i = 0; i < state->count; ++i) {
    auto workspace = &state->workspaces[i];
    if (workspace->info.id == layout.workspace) {
      workspace->layout = layout;
      state->dirty      = true;
      return;
    }
  }
}

void bar_workspace_window_added(const zdwm_window_t *window, void *user_data) {
  auto state = (bar_workspace_state_t *)user_data;

  auto win =
    array_push(state->windows, state->window_count, state->window_capacity);
  *win = *window;

  state->dirty = true;
}

static void bar_workspace_initial_windows(
  const zdwm_window_t *list,
  size_t count,
  void *user_data
) {
  for (size_t i = 0; i < count; ++i) {
    auto window = &list[i];
    bar_workspace_window_added(window, user_data);
  }
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
      break;
    }
  }

  if (!found) return;

  if (array_erase(state->windows, state->window_count, index)) {
    state->dirty = true;
  }
}

void bar_workspace_add_listeners(listeners_t *listeners, void *state) {
#define ADD(ADD_FN, LISTENER) ADD_FN(listeners, LISTENER, state)

  ADD(listeners_add_initial_workspace_listener, bar_workspace_list_filter);
  ADD(listeners_add_active_workspace_listener, bar_workspace_active_updated);
  ADD(listeners_add_layout_notify, bar_workspace_layout_notify);
  ADD(listeners_add_initial_window_listener, bar_workspace_initial_windows);
  ADD(listeners_add_window_added_listener, bar_workspace_window_added);
  ADD(listeners_add_window_updated_listener, bar_workspace_window_updated);
  ADD(listeners_add_window_removed_listener, bar_workspace_window_removed);

#undef ADD
}
