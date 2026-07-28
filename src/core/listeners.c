#include "core/listeners.h"

#include "base/memory.h"
#include "common/listeners.h"
#include "core/binding.h"
#include "core/layout.h"
#include "core/state.h"
#include "core/window.h"

#define FOR_EACH_LISTENER(FIELD, BODY)         \
  do {                                         \
    if (!(listeners)) return;                  \
    auto list = &(listeners)->FIELD;           \
    for (size_t i = 0; i < list->count; ++i) { \
      auto item = &list->items[i];             \
      BODY;                                    \
    }                                          \
  } while (false)

void listeners_notify_current_output(
  const listeners_t *listeners,
  output_id_t output_id
) {
  FOR_EACH_LISTENER(output_listeners, item->fn(output_id, item->user_data));
}

void listeners_notify_initial_workspaces(
  const listeners_t *listeners,
  const state_t *state
) {
  if (!listeners || !state) return;

  size_t count = state_workspace_count(state);
  auto items = count ? p_new(zdwm_workspace_t, count) : nullptr;
  for (size_t i = 0; i < count; ++i) {
    auto workspace = state_workspace_at(state, i);

    items[i] = (zdwm_workspace_t){
      .output_id = workspace->output_id,
      .id = workspace->id,
      .name = workspace->name,
    };
  }

  FOR_EACH_LISTENER(
    initial_workspace_listeners,
    item->fn(items, count, item->user_data)
  );

  p_delete(&items);
}

void listeners_notify_workspace_active(
  const listeners_t *listeners,
  output_id_t output_id,
  workspace_id_t workspace_id
) {
  FOR_EACH_LISTENER(
    active_workspace_listeners,
    item->fn(output_id, workspace_id, item->user_data)
  );
}

void listeners_notify_layout(
  const listeners_t *listeners,
  const layout_registry_t *layouts,
  workspace_id_t workspace_id,
  layout_id_t layout_id
) {
  if (!listeners || !layouts) return;

  auto slot = layout_slot_get(layouts, layout_id);
  if (!slot) return;

  zdwm_layout_notify_t notify = {
    .workspace = workspace_id,
    .id = layout_id,
    .name = slot->name,
    .symbol = slot->symbol,
    .description = slot->description,
  };

  FOR_EACH_LISTENER(layout_listeners, item->fn(notify, item->user_data));
}

void listeners_notify_binding_mode(
  const listeners_t *listeners,
  const binding_table_t *binding_table
) {
  if (!listeners || !binding_table) return;

  auto default_mode = binding_table_get_default_mode(binding_table);
  auto current_mode = binding_table_get_current_mode(binding_table);
  auto name = binding_table_get_mode_name(binding_table, current_mode);

  zdwm_binding_mode_notify_t notify = {
    .is_default_mode = default_mode == current_mode,
    .id = current_mode,
    .name = name,
  };

  FOR_EACH_LISTENER(binding_mode_listeners, item->fn(notify, item->user_data));
}

static bool listeners_window_snapshot(
  const state_t *state,
  window_id_t window_id,
  zdwm_window_t *out
) {
  if (!state || !out || window_id_invalid(window_id)) return false;

  auto window = state_window_get(state, window_id);
  if (!window) return false;

  auto workspace = state_workspace_get(state, window->workspace_id);
  if (!workspace) return false;

  *out = (zdwm_window_t){
    .workspace = window->workspace_id,
    .id = window->id,
    .title = window_get_showing_title(window),
    .focused = workspace->focused_window_id == window->id,
    .urgent = window->urgent,
    .skip_taskbar = window->skip_taskbar,
  };
  return true;
}

void listeners_notify_initial_windows(
  const listeners_t *listeners,
  const state_t *state
) {
  if (!listeners || !state) return;

  size_t count = state_window_count(state);
  zdwm_window_t *items = count ? p_new(zdwm_window_t, count) : nullptr;
  for (size_t i = 0; i < count; ++i) {
    auto window = state_window_at(state, i);
    if (!listeners_window_snapshot(state, window->id, &items[i])) {
      p_clear(&items[i], 1);
    }
  }

  FOR_EACH_LISTENER(
    initial_window_listeners,
    item->fn(items, count, item->user_data)
  );

  p_delete(&items);
}

void listeners_notify_window_added(
  const listeners_t *listeners,
  const state_t *state,
  window_id_t window_id
) {
  if (!listeners || !state) return;

  zdwm_window_t window = {0};
  if (!listeners_window_snapshot(state, window_id, &window)) return;

  FOR_EACH_LISTENER(window_added_listeners, item->fn(&window, item->user_data));
}

void listeners_notify_window_updated(
  const listeners_t *listeners,
  const state_t *state,
  window_id_t window_id
) {
  if (!listeners || !state) return;

  zdwm_window_t window = {0};
  if (!listeners_window_snapshot(state, window_id, &window)) return;

  FOR_EACH_LISTENER(
    window_updated_listeners,
    item->fn(&window, item->user_data)
  );
}

void listeners_notify_window_removed(
  const listeners_t *listeners,
  window_id_t window_id
) {
  if (!listeners || window_id_invalid(window_id)) return;

  FOR_EACH_LISTENER(
    window_removed_listeners,
    item->fn(window_id, item->user_data)
  );
}

#undef FOR_EACH_LISTENER
