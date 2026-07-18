#include "bar/tray.h"

#include <assert.h>
#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "base/memory.h"
#include "interface/tray.h"

typedef struct bar_tray_state_t {
  tray_t tray;
  bool dirty;
} bar_tray_state_t;

static void bar_tray_icons_changed(void *user_data) {
  bar_tray_state_t *state = user_data;
  state->dirty = true;
}

static void *bar_tray_create_state(zdwm_output_id_t output_id, void *config) {
  tray_t *tray = config;
  assert(tray != nullptr);

  auto state = p_new(bar_tray_state_t, 1);
  state->tray = *tray;
  tray->api.set_listener(tray->handle, bar_tray_icons_changed, state);

  return state;
}

static void bar_tray_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cell_api,
  void *state
) {
  bar_tray_state_t *data = state;
  if (!data->dirty) return;

  auto tray_handle = data->tray.handle;
  auto tray_api = &data->tray.api;
  auto icon_count = tray_api->icon_count(tray_handle);
  auto cell_count = cell_api->get_cell_count(item);
  if (cell_count == icon_count) {
    data->dirty = false;
    return;
  }

  cell_api->set_cell_count(item, icon_count);
  auto cell_width = tray_api->icon_size(tray_handle);
  for (size_t i = 0; i < icon_count; ++i) {
    cell_api->cell_set_fixed_width(item, i, cell_width);
  }

  data->dirty = false;
}

static void bar_tray_after_layout(const zdwm_bar_item_hook_params_t *params) {
  bar_tray_state_t *state = params->state;
  auto tray_handle = state->tray.handle;
  auto tray_api = &state->tray.api;
  tray_api->place(tray_handle, params->region.start);
}

static void bar_tray_destroy_state(void *state) {
  if (!state) return;

  bar_tray_state_t *data = state;
  p_delete(&data);
}

zdwm_bar_item_type_t bar_tray = {
  .create_state = bar_tray_create_state,
  .update = bar_tray_update,
  .after_layout = bar_tray_after_layout,
  .destroy_state = bar_tray_destroy_state,
  .update_interval_ms = 0,
};
