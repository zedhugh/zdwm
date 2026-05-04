#pragma once

#include "core/runtime.h"

void spawn(const char *command);
void quit(runtime_t *runtime, bool restart);
void raise_or_run(
  zdwm_runtime_t *runtime,
  const char *class_name,
  const char *command
);
