#include "core/state.h"

typedef struct wm_runtime_t {
  bool running;
  bool will_restart;

  wm_state_t state;
} wm_runtime_t;

bool wm_runtime_init(wm_runtime_t *runtime);
void wm_runtime_shutdown(wm_runtime_t *runtime);
