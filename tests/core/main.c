#include "core/runtime.h"

int main(void) {
  runtime_t runtime = {0};
  runtime_init(&runtime, nullptr);

  runtime_shutdown(&runtime);

  return 0;
}
