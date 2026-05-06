#pragma once

#include <stdint.h>
#include <zdwm/action.h>

void spawn(const char *command);
void quit(zdwm_runtime_t *runtime, bool restart);
void raise_or_run(
  zdwm_runtime_t *runtime,
  const char *class_name,
  const char *command
);

void toggle_fullscreen(zdwm_runtime_t *runtime);
void toggle_maximize(zdwm_runtime_t *runtime);
void toggle_floating(zdwm_runtime_t *runtime);
void toggle_sticky(zdwm_runtime_t *runtime);
void switch_workspace(
  zdwm_runtime_t *runtime,
  zdwm_workspace_id_t workspace_id
);
void cycle_layout(zdwm_runtime_t *runtime, int32_t delta);
void cycle_current_output(zdwm_runtime_t *runtime, int32_t delta);
