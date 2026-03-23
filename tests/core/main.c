#include "base/log.h"
#include "config/runtime_config.h"
#include "core/backend.h"
#include "core/runtime.h"

static void bootstrap(runtime_t *runtime) {
  backend_t *backend = backend_create(nullptr);
  backend_detect_t *detect = backend_detect(backend);

  if (!backend || !detect || detect->output_count == 0) {
    fatal("backend detect failed");
  }

  runtime_init_desc_t desc = {
    .backend = backend,
    .outputs = detect->outputs,
    .output_count = detect->output_count,
  };
  if (!runtime_config_load(nullptr, &desc)) {
    fatal("failed to build runtime inputs");
  }

  backend = nullptr;

  bool inited = runtime_init(runtime, &desc);

  runtime_init_desc_cleanup(&desc);
  backend_detect_destroy(detect);

  if (!inited) {
    fatal("zdwm runtime init failed");
  }
};

int main(void) {
  runtime_t runtime = {0};

  bootstrap(&runtime);
  runtime_shutdown(&runtime);

  return 0;
}
