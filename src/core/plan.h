#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/types.h"

typedef uint32_t dirty_mask_t;

typedef enum dirty_flags_t : dirty_mask_t {
  ZDWM_DIRTY_NONE   = 0,
  ZDWM_DIRTY_LAYOUT = 1u << 0,
} dirty_flags_t;

typedef enum effect_type_t {
  ZDWM_EFFECT_NONE,
  ZDWM_EFFECT_MAP_WINDOW,
  ZDWM_EFFECT_UNMAP_WINDOW,
  ZDWM_EFFECT_FOCUS_WINDOW,
} effect_type_t;

typedef struct effect_only_window_id_t {
  window_id_t window;
} effect_only_window_id_t;

typedef struct effect_t {
  effect_type_t type;
  union {
    effect_only_window_id_t map;
    effect_only_window_id_t unmap;
    effect_only_window_id_t focus;
  } as;
} effect_t;

typedef struct plan_t {
  effect_t *effects;
  size_t count;
  size_t capacity;
  dirty_mask_t dirty_flags;
} plan_t;

void plan_reset(plan_t *plan);
void plan_cleanup(plan_t *plan);
void plan_push_effect(plan_t *plan, const effect_t *effect);
