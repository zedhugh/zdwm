#include "core/state.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zdwm/types.h>

#include "base/array.h"
#include "base/log.h"
#include "base/memory.h"
#include "base/window_list.h"
#include "core/layer.h"
#include "core/types.h"
#include "core/window.h"
#include "core/wm_desc.h"

void state_init(
  state_t *state,
  const output_info_t *outputs,
  size_t output_count,
  const workspace_desc_t *workspaces,
  size_t workspace_count
) {
  p_clear(state, 1);

  state->outputs = p_new(output_t, output_count);
  for (size_t i = 0; i < output_count; i++) {
    const output_info_t *output_info = &outputs[i];
    output_t *output                 = &state->outputs[i];

    output->id                   = (output_id_t)i;
    output->current_workspace_id = ZDWM_WORKSPACE_ID_INVALID;
    output->name                 = p_strdup_nullable(output_info->name);
    output->geometry             = output_info->geometry;
    output->workarea             = output_info->geometry;
  }
  state->output_count = output_count;

  state->workspaces = p_new(workspace_t, workspace_count);
  for (size_t i = 0; i < workspace_count; i++) {
    const workspace_desc_t *workspace_desc = &workspaces[i];
    if (!workspace_desc_valid(workspace_desc, output_count)) {
      fatal("workspace desc at index %zu invalid", i);
    }

    output_t *output       = &state->outputs[workspace_desc->output_index];
    workspace_t *workspace = &state->workspaces[i];
    workspace->id          = (workspace_id_t)i;
    workspace->output_id   = output->id;
    workspace->focused_window_id = ZDWM_WINDOW_ID_INVALID;
    workspace->available_layouts =
      p_copy(workspace_desc->layout_ids, workspace_desc->layout_count);
    workspace->layout_id    = workspace_desc->initial_layout_id;
    workspace->layout_count = workspace_desc->layout_count;
    workspace->name         = p_strdup(workspace_desc->name);

    if (output->current_workspace_id == ZDWM_WORKSPACE_ID_INVALID) {
      output->current_workspace_id = workspace->id;
    }
  }
  state->workspace_count = workspace_count;

  for (size_t i = 0; i < state->output_count; i++) {
    const output_t *output = &state->outputs[i];
    if (output->current_workspace_id == ZDWM_WORKSPACE_ID_INVALID) {
      fatal("output at index %zu has no workspace", i);
    }
  }
}

void state_cleanup(state_t *state) {
  for (size_t i = 0; i < state->window_count; i++) {
    window_t *window = &state->windows[i];
    p_delete(&window->title);
    p_delete(&window->app_id);
    p_delete(&window->role);
    p_delete(&window->class_name);
    p_delete(&window->instance_name);
  }
  state->window_count = 0;
  p_delete(&state->windows);
  state->window_capacity = 0;

  for (size_t i = 0; i < ZDWM_WINDOW_LAYER_COUNT; ++i) {
    layer_stack_t *layer = &state->stacks[i];
    p_delete(&layer->order);
    layer->order    = 0;
    layer->capacity = 0;
  }

  for (size_t i = 0; i < state->workspace_count; i++) {
    workspace_t *workspace = &state->workspaces[i];
    p_delete(&workspace->name);
    p_delete(&workspace->available_layouts);
    workspace->layout_count = 0;
  }
  p_delete(&state->workspaces);
  state->workspace_count = 0;

  for (size_t i = 0; i < state->output_count; i++) {
    output_t *output = &state->outputs[i];
    p_delete(&output->name);
  }
  p_delete(&state->outputs);
  state->output_count = 0;

  p_clear(state, 1);
}

const workspace_t *
state_workspace_get(const state_t *state, workspace_id_t id) {
  if (id < state->workspace_count) return &state->workspaces[id];
  return nullptr;
}

const workspace_t *state_workspace_at(const state_t *state, size_t index) {
  if (index < state->workspace_count) return &state->workspaces[index];
  return nullptr;
}

static void state_workspace_adjust_focused_window(
  const state_t *state,
  workspace_id_t workspace_id
) {
  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (!workspace) return;

  const window_t *window =
    state_window_get(state, workspace->focused_window_id);
  if (window && window->workspace_id == workspace_id) return;

  for (size_t i = ZDWM_WINDOW_LAYER_TOP; i >= ZDWM_WINDOW_LAYER_NORMAL; --i) {
    const layer_stack_t *layer = &state->stacks[i];
    for (size_t i = layer->count; i > 0; --i) {
      window_id_t window_id = layer->order[i - 1];
      window                = state_window_get(state, window_id);
      if (window && window->workspace_id == workspace_id) {
        workspace->focused_window_id = window_id;
        return;
      }
    }
  }

  workspace->focused_window_id = ZDWM_WINDOW_ID_INVALID;
}

bool state_workspace_cycle_layout(state_t *state, workspace_id_t workspace_id) {
  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (!workspace) return false;

  size_t next_index = 0;
  bool matched      = false;
  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->layout_id == workspace->available_layouts[i]) {
      next_index = (i + 1) % workspace->layout_count;
      matched    = true;
      break;
    }
  }

  if (!matched) return false;

  auto next_layout_id  = workspace->available_layouts[next_index];
  workspace->layout_id = next_layout_id;
  return true;
}

bool state_workspace_set_layout_by_index(
  state_t *state,
  workspace_id_t workspace_id,
  size_t index
) {
  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (!workspace) return false;
  if (index >= workspace->layout_count) return false;

  workspace->layout_id = workspace->available_layouts[index];
  return true;
}

bool state_workspace_set_layout_by_id(
  state_t *state,
  workspace_id_t workspace_id,
  layout_id_t layout_id
) {
  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (!workspace) return false;
  for (size_t i = 0; i < workspace->layout_count; i++) {
    if (workspace->available_layouts[i] == layout_id) {
      workspace->layout_id = layout_id;
      return true;
    }
  }

  return false;
}

void state_workspace_set_focused_window(
  state_t *state,
  workspace_id_t workspace_id,
  window_id_t window_id
) {
  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (!workspace) return;

  for (size_t i = 0; i < state->window_count; i++) {
    window_t *window = &state->windows[i];
    if (window->id == window_id && window->workspace_id == workspace_id) {
      workspace->focused_window_id = window_id;
      return;
    }
  }

  state_workspace_adjust_focused_window(state, workspace_id);
}

size_t state_workspace_count(const state_t *state) {
  return state->workspace_count;
}

bool state_workspace_valid(const state_t *state, workspace_id_t id) {
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

bool state_workspace_show(const state_t *state, workspace_id_t workspace_id) {
  auto workspace = state_workspace_get(state, workspace_id);
  auto output    = state_output_get(state, workspace->output_id);
  return output->current_workspace_id == workspace_id;
}

const output_t *state_output_get(const state_t *state, output_id_t id) {
  if (id < state->output_count) return &state->outputs[id];
  return nullptr;
}

const output_t *state_output_at(const state_t *state, size_t index) {
  if (index < state->output_count) return &state->outputs[index];
  return nullptr;
}

void state_output_set_workarea(
  state_t *state,
  output_id_t output_id,
  rect_t workarea
) {
  output_t *output = (output_t *)state_output_get(state, output_id);
  if (output) output->workarea = workarea;
}

bool state_output_set_current_workspace(
  state_t *state,
  output_id_t output_id,
  workspace_id_t workspace_id,
  workspace_id_t *old_workspace_id
) {
  output_t *output = (output_t *)state_output_get(state, output_id);
  if (!output) return false;
  const workspace_t *workspace = state_workspace_get(state, workspace_id);
  if (!workspace || workspace->output_id != output_id) return false;

  if (output->current_workspace_id == workspace_id) return false;

  if (old_workspace_id) *old_workspace_id = output->current_workspace_id;
  output->current_workspace_id = workspace_id;
  return true;
}

size_t state_output_count(const state_t *state) { return state->output_count; }

bool state_output_valid(const state_t *state, output_id_t id) {
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

void state_cycle_current_output(state_t *state, int delta) {
  int64_t count   = (int64_t)state->output_count;
  int64_t current = (int64_t)state->current_output_index;
  int64_t next    = (current + (int64_t)delta) % count;
  if (next < 0) next += count;
  state->current_output_index = (size_t)next;
}

bool state_set_current_output(state_t *state, output_id_t output_id) {
  for (size_t i = 0; i < state->output_count; ++i) {
    auto output = &state->outputs[i];
    if (output->id != output_id) continue;

    if (i == state->current_output_index) return false;

    state->current_output_index = i;
    return true;
  }

  warn("state_set_current_output: invalid output_id %u", output_id);
  return false;
}

const window_t *state_window_add(state_t *state, const window_info_t *info) {
  window_id_t id   = info->id;
  window_t *window = (window_t *)state_window_get(state, id);
  if (!window) {
    window =
      array_push(state->windows, state->window_count, state->window_capacity);

    layer_stack_t *layer = &state->stacks[info->layer_type];
    layer_stack_append(layer, id);

    p_clear(window, 1);
    window->id            = id;
    window->transient_for = info->transient_for;
    window->workspace_id  = ZDWM_WORKSPACE_ID_INVALID;
    window->layer         = info->layer_type;
  }

  window_set_fullscreen(window, info->fullscreen);
  window_set_maximized(window, info->maximized);
  window_set_minimized(window, info->minimized);
  window_set_urgent(window, info->urgent);
  window_set_fixed_size(window, info->fixed_size);
  window_set_frame_rect(window, info->frame_rect);
  window_set_title(window, info->title);
  window_set_app_id(window, info->app_id);
  window_set_role(window, info->role);
  window_set_class(window, info->class_name);
  window_set_instance(window, info->instance_name);
  window_set_skip_taskbar(window, info->skip_taskbar);

  return window;
}

const window_t *state_window_get(const state_t *state, window_id_t id) {
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->windows[i].id == id) return &state->windows[i];
  }

  return nullptr;
}

const window_t *state_window_at(const state_t *state, size_t index) {
  if (index < state->window_count) return &state->windows[index];

  return nullptr;
}

void state_window_remove(state_t *state, window_id_t id) {
  bool matched = false;
  size_t index = 0;
  for (size_t i = 0; i < state->window_count; i++) {
    if (state->windows[i].id == id) {
      matched = true;
      index   = i;
      break;
    }
  }
  if (!matched) return;

  window_t *window               = &state->windows[index];
  window_layer_type_t layer_type = window->layer;
  workspace_id_t workspace_id    = window->workspace_id;
  p_delete(&window->title);
  p_delete(&window->app_id);
  p_delete(&window->role);
  p_delete(&window->class_name);
  p_delete(&window->instance_name);

  if (array_erase(state->windows, state->window_count, index)) {
    window = &state->windows[state->window_count];
    p_clear(window, 1);
    window->id            = ZDWM_WINDOW_ID_INVALID;
    window->transient_for = ZDWM_WINDOW_ID_INVALID;
    window->workspace_id  = ZDWM_WORKSPACE_ID_INVALID;

    for (size_t i = 0; i < state->window_count; ++i) {
      window = &state->windows[i];
      if (window->transient_for != id) continue;

      window->transient_for = ZDWM_WINDOW_ID_INVALID;
    }
  }

  layer_stack_t *layer = &state->stacks[layer_type];
  if (layer) {
    layer_stack_remove(layer, id);
  }

  workspace_t *workspace =
    (workspace_t *)state_workspace_get(state, workspace_id);
  if (workspace && workspace->focused_window_id == id) {
    state_workspace_adjust_focused_window(state, workspace_id);
  }
}

size_t state_window_count(const state_t *state) { return state->window_count; }

void state_window_set_workspace(
  state_t *state,
  window_id_t window_id,
  workspace_id_t workspace_id
) {
  const workspace_t *workspace = state_workspace_get(state, workspace_id);
  window_t *window             = (window_t *)state_window_get(state, window_id);
  if (!workspace || !window || window->workspace_id == workspace_id) return;

  workspace            = state_workspace_get(state, window->workspace_id);
  window->workspace_id = workspace_id;

  if (workspace && workspace->focused_window_id == window_id) {
    state_workspace_adjust_focused_window(state, workspace->id);
  }

  state_workspace_adjust_focused_window(state, workspace_id);
}

void state_window_set_fullscreen(
  state_t *state,
  window_id_t window_id,
  bool fullscreen
) {
  auto window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_fullscreen(window, fullscreen);
}

void state_window_set_maximized(
  state_t *state,
  window_id_t window_id,
  bool maximized
) {
  auto window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_maximized(window, maximized);
}

void state_window_set_minimized(
  state_t *state,
  window_id_t window_id,
  bool minimized
) {
  auto window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_minimized(window, minimized);
}

void state_window_set_floating(
  state_t *state,
  window_id_t window_id,
  bool floating
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_floating(window, floating);
}

void state_window_set_sticky(
  state_t *state,
  window_id_t window_id,
  bool sticky
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_sticky(window, sticky);
}

void state_window_set_urgent(
  state_t *state,
  window_id_t window_id,
  bool urgent
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_urgent(window, urgent);
}

void state_window_set_fixed_size(
  state_t *state,
  window_id_t window_id,
  bool fixed_size
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_fixed_size(window, fixed_size);
}

void state_window_set_border_width(
  state_t *state,
  window_id_t window_id,
  uint32_t border_width
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_border_width(window, border_width);
}

void state_window_set_skip_taskbar(
  state_t *state,
  window_id_t window_id,
  bool skip_taskbar
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_skip_taskbar(window, skip_taskbar);
}

void state_window_set_float_rect(
  state_t *state,
  window_id_t window_id,
  rect_t float_rect
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_float_rect(window, float_rect);
}

void state_window_set_frame_rect(
  state_t *state,
  window_id_t window_id,
  rect_t frame_rect
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (window) window_set_frame_rect(window, frame_rect);
}

bool state_window_take_metadata(
  state_t *state,
  window_id_t window_id,
  window_metadata_t *metadata,
  uint32_t changed_fields
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_take_metadata(window, metadata, changed_fields);

  return true;
}

bool state_window_set_title(
  state_t *state,
  window_id_t window_id,
  const char *title
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_set_title(window, title);
  return true;
}

bool state_window_set_app_id(
  state_t *state,
  window_id_t window_id,
  const char *app_id
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_set_app_id(window, app_id);
  return true;
}

bool state_window_set_role(
  state_t *state,
  window_id_t window_id,
  const char *role
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_set_role(window, role);
  return true;
}

bool state_window_set_class(
  state_t *state,
  window_id_t window_id,
  const char *class_name
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_set_class(window, class_name);
  return true;
}

bool state_window_set_instance(
  state_t *state,
  window_id_t window_id,
  const char *instance_name
) {
  window_t *window = (window_t *)state_window_get(state, window_id);
  if (!window) return false;

  window_set_instance(window, instance_name);
  return true;
}

bool state_stack_raise(state_t *state, window_id_t window_id) {
  const window_t *window = state_window_get(state, window_id);
  if (!window) return false;

  layer_stack_t *layer = &state->stacks[window->layer];
  if (!layer) return false;

  return layer_stack_raise(layer, window_id);
}

bool state_stack_lower(state_t *state, window_id_t window_id) {
  const window_t *window = state_window_get(state, window_id);
  if (!window) return false;

  layer_stack_t *layer = &state->stacks[window->layer];
  if (!layer) return false;

  return layer_stack_lower(layer, window_id);
}

void state_get_windows_need_layout_in_workspace(
  const state_t *state,
  workspace_id_t workspace_id,
  window_list_t *window_list_out
) {
  for (size_t i = 0; i < state->window_count; ++i) {
    auto window = &state->windows[i];
    if (window->workspace_id == workspace_id && window_need_layout(window)) {
      window_list_push(window_list_out, window->id);
    }
  }
}
