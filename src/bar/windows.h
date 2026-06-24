#pragma once

#include <stdint.h>
#include <zdwm/bar.h>

#include "common/listeners.h"

typedef struct bar_windows_config_t {
  uint32_t cell_padding;
  const char *bg;
  const char *fg;
  const char *focused_bg;
  const char *focused_fg;
} bar_windows_config_t;

extern zdwm_bar_item_type_t bar_windows;

void bar_windows_add_listeners(listeners_t *listeners, void *state);
