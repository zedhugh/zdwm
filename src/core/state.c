#include "core/state.h"

#include <stddef.h>
#include <string.h>

#include "core/types.h"
#include "core/wm_desc.h"
#include "utils.h"

void wm_state_init(wm_state_t *state, const wm_output_info_t *outputs,
                   size_t output_count, const wm_workspace_desc_t *workspaces,
                   size_t workspace_count) {
  p_clear(state, 1);

  state->outputs = p_new(wm_output_t, output_count);
  for (size_t i = 0; i < output_count; i++) {
    const wm_output_info_t *output_info = &outputs[i];
    wm_output_t *output = &state->outputs[i];

    output->id = (wm_output_id_t)i;
    output->current_workspace_id = WM_WORKSPACE_ID_INVALID;
    output->name = p_strdup(output_info->name);
    output->geometry = output_info->geometry;
    output->workarea = output_info->geometry;
  }
  state->output_count = output_count;

  state->workspaces = p_new(wm_workspace_t, workspace_count);
  for (size_t i = 0; i < workspace_count; i++) {
    const wm_workspace_desc_t *workspace_desc = &workspaces[i];
    if (!wm_workspace_desc_valid(workspace_desc, output_count)) {
      fatal("workspace desc at index %zu invalid", i);
    }

    wm_output_t *output = &state->outputs[workspace_desc->output_index];
    wm_workspace_t *workspace = &state->workspaces[i];
    workspace->id = (wm_workspace_id_t)i;
    workspace->output_id = output->id;
    workspace->focused_window_id = WM_WINDOW_ID_INVALID;
    workspace->available_layouts =
      p_copy(workspace_desc->layout_ids, workspace_desc->layout_count);
    workspace->layout_id = workspace_desc->initial_layout_id;
    workspace->layout_count = workspace_desc->layout_count;
    workspace->name = p_strdup(workspace_desc->name);

    if (output->current_workspace_id == WM_WORKSPACE_ID_INVALID) {
      output->current_workspace_id = workspace->id;
    }
  }
  state->workspace_count = workspace_count;

  for (size_t i = 0; i < state->output_count; i++) {
    const wm_output_t *output = &state->outputs[i];
    if (output->current_workspace_id == WM_WORKSPACE_ID_INVALID) {
      fatal("output at index %zu has no workspace", i);
    }
  }
}

void wm_state_cleanup(wm_state_t *state) {
  for (size_t i = 0; i < state->window_count; i++) {
    wm_window_t *window = &state->windows[i];
    p_delete(&window->title);
    p_delete(&window->app_id);
    p_delete(&window->class_name);
    p_delete(&window->instance_name);
  }
  state->window_count = 0;
  p_delete(&state->windows);
  p_delete(&state->stack_order);
  state->window_capacity = 0;

  for (size_t i = 0; i < state->workspace_count; i++) {
    wm_workspace_t *workspace = &state->workspaces[i];
    p_delete(&workspace->name);
    p_delete(&workspace->available_layouts);
    workspace->layout_count = 0;
  }
  p_delete(&state->workspaces);
  state->workspace_count = 0;

  for (size_t i = 0; i < state->output_count; i++) {
    wm_output_t *output = &state->outputs[i];
    p_delete(&output->name);
  }
  p_delete(&state->outputs);
  state->output_count = 0;

  p_clear(state, 1);
}

const wm_workspace_t *wm_state_workspace_get(const wm_state_t *state,
                                             wm_workspace_id_t id) {
  if (id < state->workspace_count) return &state->workspaces[id];
  return nullptr;
}

const wm_workspace_t *wm_state_workspace_at(const wm_state_t *state,
                                            size_t index) {
  if (index < state->workspace_count) return &state->workspaces[index];
  return nullptr;
}

static void wm_state_workspace_adjust_focused_window(
  const wm_state_t *state, wm_workspace_id_t workspace_id) {
  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (!workspace) return;

  const wm_window_t *window =
    wm_state_window_get(state, workspace->focused_window_id);
  if (window && window->workspace_id == workspace_id) return;

  for (size_t i = state->window_count; i > 0; i--) {
    wm_window_id_t window_id = state->stack_order[i - 1];
    const wm_window_t *window = wm_state_window_get(state, window_id);
    if (window && window->workspace_id == workspace_id) {
      workspace->focused_window_id = window_id;
      return;
    }
  }

  workspace->focused_window_id = WM_WINDOW_ID_INVALID;
}

bool wm_state_workspace_cycle_layout(wm_state_t *state,
                                     wm_workspace_id_t workspace_id) {
  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (!workspace) return false;

  size_t next_index = 0;
  bool matched = false;
  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->layout_id == workspace->available_layouts[i]) {
      next_index = (i + 1) % workspace->layout_count;
      matched = true;
      break;
    }
  }

  if (!matched) return false;

  auto next_layout_id = workspace->available_layouts[next_index];
  workspace->layout_id = next_layout_id;
  return true;
}

bool wm_state_workspace_set_layout_by_index(wm_state_t *state,
                                            wm_workspace_id_t workspace_id,
                                            size_t index) {
  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (!workspace) return false;
  if (index >= workspace->layout_count) return false;

  workspace->layout_id = workspace->available_layouts[index];
  return true;
}

bool wm_state_workspace_set_layout_by_id(wm_state_t *state,
                                         wm_workspace_id_t workspace_id,
                                         wm_layout_id_t layout_id) {
  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (!workspace) return false;
  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->available_layouts[i] == layout_id) {
      workspace->layout_id = layout_id;
      return true;
    }
  }

  return false;
}

void wm_state_workspace_set_focused_window(wm_state_t *state,
                                           wm_workspace_id_t workspace_id,
                                           wm_window_id_t window_id) {
  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (!workspace) return;

  for (size_t i = 0; i < state->window_count; i++) {
    wm_window_t *window = &state->windows[i];
    if (window->id == window_id && window->workspace_id == workspace_id) {
      workspace->focused_window_id = window_id;
      return;
    }
  }

  wm_state_workspace_adjust_focused_window(state, workspace_id);
}

size_t wm_state_workspace_count(const wm_state_t *state) {
  return state->workspace_count;
}

bool wm_state_workspace_valid(const wm_state_t *state, wm_workspace_id_t id) {
  return id < state->workspace_count;

  /* 线性扫描作为备用 */
#if 0
  if (id >= state->workspace_count) return false;

  for (size_t i = 0; i < state->workspace_count; i++) {
    if (state->workspaces[i].id == id) return true;
  }

  return false;
#endif
}

const wm_output_t *wm_state_output_get(const wm_state_t *state,
                                       wm_output_id_t id) {
  if (id < state->output_count) return &state->outputs[id];
  return nullptr;
}

const wm_output_t *wm_state_output_at(const wm_state_t *state, size_t index) {
  if (index < state->output_count) return &state->outputs[index];
  return nullptr;
}

void wm_state_output_set_workarea(wm_state_t *state, wm_output_id_t output_id,
                                  wm_rect_t workarea) {
  wm_output_t *output = (wm_output_t *)wm_state_output_get(state, output_id);
  if (output) output->workarea = workarea;
}

void wm_state_output_set_current_workspace(wm_state_t *state,
                                           wm_output_id_t output_id,
                                           wm_workspace_id_t workspace_id) {
  wm_output_t *output = (wm_output_t *)wm_state_output_get(state, output_id);
  if (!output) return;
  const wm_workspace_t *workspace = wm_state_workspace_get(state, workspace_id);
  if (!workspace || workspace->output_id != output_id) return;

  output->current_workspace_id = workspace_id;
}

size_t wm_state_output_count(const wm_state_t *state) {
  return state->output_count;
}

bool wm_state_output_valid(const wm_state_t *state, wm_output_id_t id) {
  return id < state->output_count;

  /* 线性扫描作为备用 */
#if 0
  if (id >= state->output_count) return false;

  for (size_t i = 0; i < state->output_count; i++) {
    if (state->outputs[i].id == id) return true;
  }

  return false;
#endif
}

const wm_window_t *wm_state_window_add(wm_state_t *state,
                                       const wm_window_info_t *info) {
  wm_window_id_t id = info->id;
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, id);
  if (!window) {
    if (state->window_count == state->window_capacity) {
      size_t capacity = next_capacity(state->window_capacity);
      p_realloc(&state->windows, capacity);
      p_realloc(&state->stack_order, capacity);
      state->window_capacity = capacity;
    }
    window = &state->windows[state->window_count];
    p_clear(window, 1);
    window->id = id;
    window->workspace_id = WM_WORKSPACE_ID_INVALID;

    state->stack_order[state->window_count] = id;
    state->window_count++;
  }

  wm_state_window_set_geometry_mode(state, id, info->geometry_mode);
  wm_state_window_set_urgent(state, id, info->urgent);
  wm_state_window_set_fixed_size(state, id, info->fixed_size);
  wm_state_window_set_frame_rect(state, id, info->frame_rect);
  wm_state_window_set_title(state, id, info->title);
  wm_state_window_set_app_id(state, id, info->app_id);
  wm_state_window_set_class(state, id, info->class_name);
  wm_state_window_set_instance(state, id, info->instance_name);
  wm_state_window_set_skip_taskbar(state, id, info->skip_taskbar);

  return window;
}

const wm_window_t *wm_state_window_get(const wm_state_t *state,
                                       wm_window_id_t id) {
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->windows[i].id == id) return &state->windows[i];
  }

  return nullptr;
}

const wm_window_t *wm_state_window_at(const wm_state_t *state, size_t index) {
  if (index < state->window_count) return &state->windows[index];

  return nullptr;
}

void wm_state_window_remove(wm_state_t *state, wm_window_id_t id) {
  bool matched = false;
  size_t index = 0;
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->windows[i].id == id) {
      matched = true;
      index = i;
      break;
    }
  }
  if (!matched) return;

  wm_window_t *window = &state->windows[index];
  wm_workspace_id_t workspace_id = window->workspace_id;
  p_delete(&window->title);
  p_delete(&window->app_id);
  p_delete(&window->class_name);
  p_delete(&window->instance_name);

  for (size_t i = index + 1; i < state->window_count; i++) {
    state->windows[i - 1] = state->windows[i];
  }
  window = &state->windows[state->window_count - 1];
  p_clear(window, 1);
  window->id = WM_WINDOW_ID_INVALID;
  window->workspace_id = WM_WORKSPACE_ID_INVALID;

  matched = false;
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->stack_order[i] == id) {
      matched = true;
      index = i;
      break;
    }
  }
  if (matched) {
    for (size_t i = index + 1; i < state->window_count; i++) {
      state->stack_order[i - 1] = state->stack_order[i];
    }
    state->stack_order[state->window_count - 1] = WM_WINDOW_ID_INVALID;
  }

  state->window_count--;

  wm_workspace_t *workspace =
    (wm_workspace_t *)wm_state_workspace_get(state, workspace_id);
  if (workspace && workspace->focused_window_id == id) {
    wm_state_workspace_adjust_focused_window(state, workspace_id);
  }
}

size_t wm_state_window_count(const wm_state_t *state) {
  return state->window_count;
}

void wm_state_window_set_workspace(wm_state_t *state, wm_window_id_t window_id,
                                   wm_workspace_id_t workspace_id) {
  const wm_workspace_t *workspace = wm_state_workspace_get(state, workspace_id);
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!workspace || !window || window->workspace_id == workspace_id) return;

  workspace = wm_state_workspace_get(state, window->workspace_id);
  window->workspace_id = workspace_id;

  if (workspace && workspace->focused_window_id == window_id) {
    wm_state_workspace_adjust_focused_window(state, workspace->id);
  }

  wm_state_workspace_adjust_focused_window(state, workspace_id);
}

void wm_state_window_set_geometry_mode(
  wm_state_t *state, wm_window_id_t window_id,
  wm_window_geometry_mode_t geometry_mode) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->geometry_mode = geometry_mode;
}

void wm_state_window_set_floating(wm_state_t *state, wm_window_id_t window_id,
                                  bool floating) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->floating = floating;
}

void wm_state_window_set_sticky(wm_state_t *state, wm_window_id_t window_id,
                                bool sticky) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->sticky = sticky;
  if (window->sticky) wm_state_window_set_floating(state, window_id, true);
}

void wm_state_window_set_urgent(wm_state_t *state, wm_window_id_t window_id,
                                bool urgent) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->urgent = urgent;
}

void wm_state_window_set_fixed_size(wm_state_t *state, wm_window_id_t window_id,
                                    bool fixed_size) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->fixed_size = fixed_size;
  if (fixed_size) wm_state_window_set_floating(state, window_id, true);
}

void wm_state_window_set_skip_taskbar(wm_state_t *state,
                                      wm_window_id_t window_id,
                                      bool skip_taskbar) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->skip_taskbar = skip_taskbar;
}

void wm_state_window_set_float_rect(wm_state_t *state, wm_window_id_t window_id,
                                    wm_rect_t float_rect) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->float_rect = float_rect;
}

void wm_state_window_set_frame_rect(wm_state_t *state, wm_window_id_t window_id,
                                    wm_rect_t frame_rect) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return;

  window->frame_rect = frame_rect;
}

bool wm_state_window_set_title(wm_state_t *state, wm_window_id_t window_id,
                               const char *title) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return false;

  p_delete(&window->title);
  if (title) window->title = p_strdup(title);

  return true;
}

bool wm_state_window_set_app_id(wm_state_t *state, wm_window_id_t window_id,
                                const char *app_id) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return false;

  p_delete(&window->app_id);
  if (app_id) window->app_id = p_strdup(app_id);

  return true;
}
bool wm_state_window_set_class(wm_state_t *state, wm_window_id_t window_id,
                               const char *class_name) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return false;

  p_delete(&window->class_name);
  if (class_name) window->class_name = p_strdup(class_name);

  return true;
}
bool wm_state_window_set_instance(wm_state_t *state, wm_window_id_t window_id,
                                  const char *instance_name) {
  wm_window_t *window = (wm_window_t *)wm_state_window_get(state, window_id);
  if (!window) return false;

  p_delete(&window->instance_name);
  if (instance_name) window->instance_name = p_strdup(instance_name);

  return true;
}

const wm_window_id_t *wm_state_stack_order(const wm_state_t *state) {
  return state->stack_order;
}

wm_window_id_t wm_state_stack_at(const wm_state_t *state, size_t index) {
  if (index >= state->window_count) return WM_WINDOW_ID_INVALID;
  return state->stack_order[index];
}

bool wm_state_stack_raise(wm_state_t *state, wm_window_id_t window_id) {
  bool matched = false;
  size_t index = 0;
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->stack_order[i] == window_id) {
      matched = true;
      index = i;
      break;
    }
  }

  if (!matched) return false;

  for (size_t i = index + 1; i < state->window_count; i++) {
    state->stack_order[i - 1] = state->stack_order[i];
  }
  state->stack_order[state->window_count - 1] = window_id;

  return true;
}

bool wm_state_stack_lower(wm_state_t *state, wm_window_id_t window_id) {
  bool matched = false;
  size_t index = 0;
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->stack_order[i] == window_id) {
      matched = true;
      index = i;
      break;
    }
  }

  if (!matched) return false;

  for (size_t i = index; i > 0; i--) {
    state->stack_order[i] = state->stack_order[i - 1];
  }
  state->stack_order[0] = window_id;

  return true;
}
