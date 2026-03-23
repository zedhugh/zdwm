#pragma once

#include <zdwm/config.h>

typedef struct config_loader_t {
  char *path;
  void *handle;
  zdwm_config_setup_fn *setup;
} config_loader_t;

bool config_loader_load(config_loader_t *loader, const char *override_path);
void config_loader_cleanup(config_loader_t *loader);
