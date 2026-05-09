#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "base/log.h"
#include "config/runtime_config.h"
#include "core/backend.h"
#include "core/runtime.h"

static void bootstrap(runtime_t *runtime) {
  auto backend = backend_create(nullptr);
  auto detect = backend_detect(backend);
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
    fatal("runtime init failed");
  }
}

char **cmd_argv = nullptr;

int main(int argc, char *argv[]) {
  cmd_argv = argv;

  runtime_t runtime = {0};

  bootstrap(&runtime);
  runtime_setup(&runtime);
  runtime_scan(&runtime);
  runtime_run(&runtime);
  runtime_shutdown(&runtime);

  if (runtime.will_restart) {
    execvp(cmd_argv[0], cmd_argv);
    fatal("execvp() failed: %s", strerror(errno));
  }

  return 0;
}
