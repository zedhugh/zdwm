#pragma once

#include <glib.h>

typedef struct zdwm_t {
  bool need_restart;
  GMainLoop *loop;
} zdwm_t;
