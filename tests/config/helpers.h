#pragma once

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static inline void config_test_clear_env(void) {
  assert(unsetenv("ZDWM_CONFIG_LIB") == 0);
  assert(unsetenv("XDG_CONFIG_HOME") == 0);
  assert(unsetenv("HOME") == 0);
}

static inline void config_test_make_temp_dir(char *path, size_t path_size,
                                             const char *prefix) {
  int len = snprintf(path, path_size, "/tmp/%s-XXXXXX", prefix);
  assert(len > 0);
  assert((size_t)len < path_size);
  assert(mkdtemp(path) == path);
}

static inline void config_test_join_path(char *out, size_t out_size,
                                         const char *base,
                                         const char *suffix) {
  int len = snprintf(out, out_size, "%s/%s", base, suffix);
  assert(len > 0);
  assert((size_t)len < out_size);
}

static inline void config_test_mkdir(const char *path) {
  assert(mkdir(path, 0700) == 0);
}

static inline void config_test_symlink(const char *target, const char *path) {
  assert(symlink(target, path) == 0);
}

static inline void config_test_unlink(const char *path) {
  assert(unlink(path) == 0);
}

static inline void config_test_rmdir(const char *path) {
  assert(rmdir(path) == 0);
}
