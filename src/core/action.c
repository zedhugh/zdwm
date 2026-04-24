#include "core/action.h"

#include <paths.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/log.h"

void spawn(const char *command) {
  if (fork() == 0) {
    setsid();

    if (fork() == 0) {
      execl(_PATH_BSHELL, _PATH_BSHELL, "-c", command, nullptr);
      fatal("execl fail: %s", command);
    }

    exit(0);
  }

  wait(0);
}
