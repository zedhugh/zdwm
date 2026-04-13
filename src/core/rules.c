#include "core/rules.h"

#include <stddef.h>
#include <string.h>
#include <zdwm/types.h>

#include "base/memory.h"
#include "core/types.h"

bool rules_move(rules_t *src, rules_t *dest) {
  if (!dest || !src) return false;

  dest->items    = src->items;
  dest->count    = src->count;
  dest->capacity = src->capacity;

  src->items    = nullptr;
  src->count    = 0;
  src->capacity = 0;

  return true;
}

void rules_cleanup(rules_t *rules) {
  for (size_t i = 0; i < rules->count; ++i) {
    rule_match_t *match   = &rules->items[i].match;
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
  rules->count    = 0;
}

static inline bool str_match(const char *pattern, const char *value) {
  return !pattern || (value && strcmp(pattern, value) == 0);
}

static bool
rule_match_window(const rule_match_t *match, const window_metadata_t *meta) {
  return str_match(match->app_id, meta->app_id) &&
         str_match(match->role, meta->role) &&
         str_match(match->class_name, meta->class_name) &&
         str_match(match->instance_name, meta->instance_name);
}

static void rule_action_merge(const rule_action_t *src, rule_action_t *dest) {
  if (!src || !dest) return;

  if (src->workspace != ZDWM_WORKSPACE_ID_INVALID) {
    dest->workspace = src->workspace;
  }
  dest->switch_to_workspace |= src->switch_to_workspace;
  dest->fullscreen          |= src->fullscreen;
  dest->maximize            |= src->maximize;
  dest->floating            |= src->floating;
}

bool rules_resolve(
  const rules_t *rules,
  const window_metadata_t *metadata,
  rule_action_t *action_out
) {
  if (!action_out) return false;

  bool matched = false;

  for (size_t i = 0; i < rules->count; ++i) {
    if (!rule_match_window(&rules->items[i].match, metadata)) continue;

    matched = true;
    rule_action_merge(&rules->items[i].action, action_out);
  }

  return matched;
}
