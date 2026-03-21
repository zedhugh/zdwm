#pragma once

#include <stdbool.h>

typedef struct wm_policy_config_t {
  bool focus_raises;
  bool pointer_enter_focuses_window;
  bool sticky_windows_participate_in_direction_focus;
  bool manage_sets_focus;
  bool switch_workspace_restores_last_focus;
  bool minimize_clears_focus;
} wm_policy_config_t;

void wm_policy_config_init_default(wm_policy_config_t *config);
