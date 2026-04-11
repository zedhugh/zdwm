#include "core/rules.h"

#include "base/memory.h"
#include "core/types.h"

bool rules_move(rules_t *src, rules_t *dest) {
  if (!dest || !src) return false;

  dest->items = src->items;
  dest->count = src->count;
  dest->capacity = src->capacity;

  src->items = nullptr;
  src->count = 0;
  src->capacity = 0;

  return true;
}

void rules_cleanup(rules_t *rules) {
  for (size_t i = 0; i < rules->count; ++i) {
    rule_match_t *match = &rules->items[i].match;
    rule_action_t *action = &rules->items[i].action;

    p_delete(&match->app_id);
    p_delete(&match->role);
    p_delete(&match->class_name);
    p_delete(&match->instance_name);

    p_clear(action, 1);
    action->workspace = ZDWM_WORKSPACE_ID_INVALID;
  }

  p_delete(&rules->items);
  rules->capacity = 0;
  rules->count = 0;
}
