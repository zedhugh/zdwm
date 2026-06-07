#pragma once

#include <stdint.h>
#include <zdwm/bar.h>

#include "core/listeners.h"

typedef struct bar_workspace_config_t {
  uint32_t cell_padding;
  uint32_t indicator_width;
  const char *bg;
  const char *fg;
  const char *active_bg;
  const char *active_fg;
  const char *urgent_bg;
  const char *urgent_fg;
} bar_workspace_config_t;

extern zdwm_bar_item_type_t bar_workspace;

void bar_workspace_add_listeners(listeners_t *listeners, void *state);
