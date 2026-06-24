#include "bar/binding.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/listeners.h>
#include <zdwm/types.h>

#include "base/memory.h"
#include "core/listeners.h"

typedef struct bar_binding_state_t {
  zdwm_binding_mode_notify_t mode;

  bar_binding_config_t config;

  bool inited;
  bool dirty;
} bar_binding_state_t;

static void *
bar_binding_create_state(zdwm_output_id_t output_id, void *config) {
  const bar_binding_config_t *bar_config = config;
  assert(bar_config);

  auto state = p_new(bar_binding_state_t, 1);
  state->config = *bar_config;

  return state;
}

static void bar_binding_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cells,
  void *state
) {
  auto data = (bar_binding_state_t *)state;
  if (!data->inited || !data->dirty) return;

  auto config = &data->config;
  auto mode = &data->mode;

  if (!config->show_default && mode->is_default_mode) {
    cells->set_cell_count(item, 0);
  } else {
    constexpr auto index = 0;
    cells->set_cell_count(item, 1);
    cells->cell_set_text(item, index, mode->name);
    cells->cell_set_bg(item, index, config->bg);
    cells->cell_set_fg(item, index, config->fg);
  }

  data->dirty = false;
}

static zdwm_action_t bar_binding_on_click(zdwm_bar_click_params_t *params) {
  return (zdwm_action_t){
    .type = ZDWM_ACTION_BINDING_MODE_CYCLE,
    .as.binding_mode_cycle = {.delta = 1},
  };
}

static void bar_binding_destroy_state(void *state) {
  if (!state) return;

  auto data = (bar_binding_state_t *)state;
  p_delete(&data);
}

zdwm_bar_item_type_t bar_binding = {
  .create_state = bar_binding_create_state,
  .update = bar_binding_update,
  .on_click = bar_binding_on_click,
  .destroy_state = bar_binding_destroy_state,
  .update_interval_ms = 0,
};

static void
bar_binding_mode_notify(zdwm_binding_mode_notify_t mode, void *user_data) {
  auto state = (bar_binding_state_t *)user_data;
  if (!state->inited) {
    state->mode = mode;

    state->inited = true;
    state->dirty = true;
    return;
  }

  if (state->mode.id == mode.id) return;
  state->mode = mode;

  state->dirty = true;
}

void bar_binding_add_listeners(listeners_t *listeners, void *state) {
  listeners_add_binding_mode_notify(listeners, bar_binding_mode_notify, state);
}
