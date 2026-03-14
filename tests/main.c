#include "core/runtime.h"

int main(void) {
  wm_runtime_t runtime = {0};
  wm_runtime_init(&runtime);

  wm_runtime_shutdown(&runtime);

  return 0;
}
