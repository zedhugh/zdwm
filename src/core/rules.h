#pragma once

#include <stddef.h>

#include "core/types.h"

typedef struct rule_item_t {
  rule_match_t match;
  rule_action_t action;
} rule_item_t;

typedef struct rules_t {
  rule_item_t *items;
  size_t count;
  size_t capacity;
} rules_t;

bool rules_move(rules_t *src, rules_t *dest);
void rules_cleanup(rules_t *rules);
