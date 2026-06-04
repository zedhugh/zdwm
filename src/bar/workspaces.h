#pragma once

#include <stddef.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/listeners.h>
#include <zdwm/types.h>

typedef struct bar_workspace_palette_t {
  const char *bg;
  const char *fg;
  const char *active_bg;
  const char *active_fg;
  const char *urgent_bg;
  const char *urgent_fg;
} bar_workspace_palette_t;

typedef struct bar_workspace_t {
  zdwm_workspace_t info;
  size_t window_count;
  size_t urgent_window_count;
} bar_workspace_t;

typedef struct bar_workspace_state_t {
  zdwm_output_id_t output_id;
  zdwm_workspace_id_t workspace_id;
  bar_workspace_t *workspaces;
  size_t count;
  size_t capacity;

  zdwm_window_t *windows;
  size_t window_count;
  size_t window_capacity;

  bar_workspace_palette_t palette;

  bool list_inited;
  bool active_inited;
  bool dirty;
} bar_workspace_state_t;

void *bar_workspaces_create_state(zdwm_output_id_t output_id, void *config);
void bar_workspaces_update(
  zdwm_bar_item_t *item,
  const zdwm_bar_cell_api_t *cell_api,
  void *state
);
zdwm_action_t bar_workspace_on_click(
  zdwm_bar_item_t *item,
  size_t cell_index,
  int32_t x,
  void *state
);
void bar_workspace_destroy_state(void *state);

void bar_workspace_list_filter(
  const zdwm_workspace_t *list,
  size_t count,
  void *user_data
);
void bar_workspace_active_updated(
  zdwm_output_id_t output_id,
  zdwm_workspace_id_t workspace_id,
  void *user_data
);
void bar_workspace_initial_windows(
  const zdwm_window_t *list,
  size_t count,
  void *user_data
);
void bar_workspace_window_added(const zdwm_window_t *window, void *user_data);
void bar_workspace_window_updated(const zdwm_window_t *window, void *user_data);
void bar_workspace_window_removed(zdwm_window_id_t window_id, void *user_data);
