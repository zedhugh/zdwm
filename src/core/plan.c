#include "core/plan.h"

#include <stddef.h>

#include "base/array.h"
#include "base/memory.h"

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
  plan->count    = 0;
  plan->capacity = 0;
}

void plan_push_effect(plan_t *plan, const effect_t *effect) {
  effect_t *eft = array_push(plan->effects, plan->count, plan->capacity);
  *eft          = *effect;
}
