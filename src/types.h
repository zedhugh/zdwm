#pragma once

#include <glib.h>

typedef struct global_state_t {
  bool need_restart;
  GMainLoop *loop;
} global_state_t;
