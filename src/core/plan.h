#pragma once

#include <stddef.h>

#include "base/color.h"
#include "interface/effect.h"
#include "interface/types.h"

typedef struct plan_t {
  effect_t *effects;
  size_t count;
  size_t capacity;
  bool need_relayout;
  bool quit;
  bool will_restart;
} plan_t;

void plan_reset(plan_t *plan);
void plan_cleanup(plan_t *plan);
void plan_push_effect(plan_t *plan, const effect_t *effect);

void plan_push_map_effect(plan_t *plan, window_id_t window_id);
void plan_push_unmap_effect(plan_t *plan, window_id_t window_id);
void plan_push_focus_effect(plan_t *plan, window_id_t window_id);
void plan_push_kill_effect(plan_t *plan, window_id_t window_id);
void plan_push_withdraw_effect(plan_t *plan, window_id_t window_id);
void plan_push_move_effect(plan_t *plan, window_id_t window_id);
void plan_push_resize_effect(plan_t *plan, window_id_t window_id);
void plan_push_fullscreen_effect(
  plan_t *plan,
  window_id_t window_id,
  bool value
);
void plan_push_maximize_effect(plan_t *plan, window_id_t window_id, bool value);
void plan_push_minimize_effect(plan_t *plan, window_id_t window_id, bool value);
void plan_push_change_border_color_effect(
  plan_t *plan,
  window_id_t window_id,
  const color_t *color
);
void plan_push_ungrab_pointer(plan_t *plan);
