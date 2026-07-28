#include "core/plan.h"

#include <stddef.h>

#include "base/array.h"
#include "base/memory.h"
#include "interface/effect.h"
#include "interface/types.h"

static void free_memory_hold_by_effects(effect_t *effects, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    auto effect = &effects[i];

    switch (effect->type) {
    case ZDWM_EFFECT_CHANGE_WINDOW_LIST:
      p_delete(&effect->as.change_window_list.windows);
      break;
    case ZDWM_EFFECT_RESTACK_WINDOWS:
      p_delete(&effect->as.restack_windows.windows);
      break;
    case ZDWM_EFFECT_BIND_KEY:
      p_delete(&effect->as.bind_key.keys);
      break;
    case ZDWM_EFFECT_GRAB_BUTTON:
      p_delete(&effect->as.grab_button.buttons);
    default:
    }
  }
}

void plan_reset(plan_t *plan) {
  if (!plan->effects || !plan->capacity) return;

  free_memory_hold_by_effects(plan->effects, plan->count);
  p_clear(plan->effects, plan->capacity);
  plan->count = 0;
}

void plan_cleanup(plan_t *plan) {
  free_memory_hold_by_effects(plan->effects, plan->count);
  p_delete(&plan->effects);
  plan->count = 0;
  plan->capacity = 0;
  plan->need_relayout = false;
  plan->quit = false;
  plan->will_restart = false;
}

void plan_push_effect(plan_t *plan, const effect_t *effect) {
  effect_t *eft = array_push(plan->effects, plan->count, plan->capacity);
  *eft = *effect;
}

void plan_push_map_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_MAP_WINDOW,
    .as.map.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_unmap_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_UNMAP_WINDOW,
    .as.unmap.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_focus_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_FOCUS_WINDOW,
    .as.focus.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_kill_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_KILL_WINDOW,
    .as.kill.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_withdraw_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_WITHDRAW_WINDOW,
    .as.withdraw.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_move_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_START_MOVE_WINDOW,
    .as.move.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_resize_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_START_RESIZE_WINDOW,
    .as.resize.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_configure_notify_effect(plan_t *plan, window_id_t window_id) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_CONFIGURE_NOTIFY,
    .as.configure_notify.window = window_id,
  };
  plan_push_effect(plan, &effect);
}

void plan_push_fullscreen_effect(
  plan_t *plan,
  window_id_t window_id,
  bool value
) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_FULLSCREEN_WINDOW,
    .as.fullscreen = {
      .window = window_id,
      .value = value,
    },
  };
  plan_push_effect(plan, &effect);
}

void plan_push_maximize_effect(
  plan_t *plan,
  window_id_t window_id,
  bool value
) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_MAXIMIZE_WINDOW,
    .as.maximize = {
      .window = window_id,
      .value = value,
    },
  };
  plan_push_effect(plan, &effect);
}

void plan_push_minimize_effect(
  plan_t *plan,
  window_id_t window_id,
  bool value
) {
  if (window_id_invalid(window_id)) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_MINIMIZE_WINDOW,
    .as.minimize = {
      .window = window_id,
      .value = value,
    },
  };
  plan_push_effect(plan, &effect);
}

void plan_push_change_border_color_effect(
  plan_t *plan,
  window_id_t window_id,
  const color_t *color
) {
  if (window_id_invalid(window_id) || color == nullptr) return;

  effect_t effect = {
    .type = ZDWM_EFFECT_CHANGE_BORDER_COLOR,
    .as.change_border_color = {
      .window = window_id,
      .color = color,
    },
  };
  plan_push_effect(plan, &effect);
}

void plan_push_ungrab_pointer(plan_t *plan) {
  effect_t effect = {.type = ZDWM_EFFECT_UNGRAB_POINTER};
  plan_push_effect(plan, &effect);
}
