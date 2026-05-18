#pragma once

#include <stdint.h>
#include <zdwm/action.h>

typedef struct runtime_t runtime_t;

void spawn(const char *command);
void quit(runtime_t *runtime, bool restart);
void raise_or_run(
  runtime_t *runtime,
  const char *class_name,
  const char *command
);

void toggle_fullscreen(runtime_t *runtime);
void toggle_maximize(runtime_t *runtime);
void toggle_floating(runtime_t *runtime);
void toggle_sticky(runtime_t *runtime);
void switch_workspace(runtime_t *runtime, zdwm_workspace_id_t workspace_id);
void switch_workspace_in_current_output(
  runtime_t *runtime,
  zdwm_workspace_id_t workspace_id
);
void switch_workspace_by_index_in_current_output(
  runtime_t *runtime,
  uint32_t index
);
void cycle_layout(runtime_t *runtime, int32_t delta);
void cycle_current_output(runtime_t *runtime, int32_t delta);
void focus_window(runtime_t *runtime, int32_t delta);
