#pragma once

#include <stdint.h>
#include <xcb/xproto.h>
#include "types.h"

typedef struct wallpaper_context_t wallpaper_context_t;

typedef struct wallpaper_config_t {
  uint32_t interval;
  uint32_t path_count;
  char **paths;
  xcb_screen_t *screen;
  xcb_connection_t *conn;
  monitor_t *monitors;
} wallpaper_config_t;

wallpaper_context_t *wallpaper_init(wallpaper_config_t config);
void wallpaper_clean(wallpaper_context_t *context);
