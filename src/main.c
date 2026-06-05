#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "base/log.h"
#include "runtime/runtime.h"

char **cmd_argv = nullptr;

int main(int argc, char *argv[]) {
  cmd_argv = argv;

  auto restart = runtime_run(nullptr, nullptr);

  if (restart) {
    execvp(cmd_argv[0], cmd_argv);
    fatal("execvp() failed: %s", strerror(errno));
  }

  return 0;
}
