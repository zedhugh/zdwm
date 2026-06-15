#pragma once

#include <stdint.h>
#include <zdwm/bar.h>

#include "core/listeners.h"

typedef struct bar_binding_config_t {
  bool show_default;
  uint32_t cell_padding;
  const char *bg;
  const char *fg;
} bar_binding_config_t;

extern zdwm_bar_item_type_t bar_binding;

void bar_binding_add_listeners(listeners_t *listeners, void *state);
