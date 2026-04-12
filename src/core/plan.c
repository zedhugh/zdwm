#include "core/plan.h"

#include "base/array.h"
#include "base/memory.h"

void plan_reset(plan_t *plan) {
  if (!plan->effects || !plan->capacity) return;

  p_clear(plan->effects, plan->capacity);
  plan->count = 0;
}

void plan_cleanup(plan_t *plan) {
  p_delete(&plan->effects);
  plan->count = 0;
  plan->capacity = 0;
}

void plan_push_effect(plan_t *plan, const effect_t *effect) {
  effect_t *eft = array_push(plan->effects, plan->count, plan->capacity);
  *eft = *effect;
}
