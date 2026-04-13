#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "config/loader.h"
#include "helpers.h"

static void test_config_loader_loads_explicit_override(
  const char *fixture_path
) {
  config_test_clear_env();

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, fixture_path));
  assert(loader.path);
  assert(strcmp(loader.path, fixture_path) == 0);
  assert(loader.handle);
  assert(loader.setup);

  config_loader_cleanup(&loader);
}

static void test_config_loader_override_has_priority_over_env(
  const char *fixture_path
) {
  config_test_clear_env();
  assert(
    setenv("ZDWM_CONFIG_LIB", "/tmp/definitely-missing-config.so", 1) == 0
  );

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, fixture_path));
  assert(loader.path);
  assert(strcmp(loader.path, fixture_path) == 0);
  assert(loader.handle);
  assert(loader.setup);

  config_loader_cleanup(&loader);
}

static void test_config_loader_uses_zdwm_config_lib_env(
  const char *fixture_path
) {
  config_test_clear_env();
  assert(setenv("ZDWM_CONFIG_LIB", fixture_path, 1) == 0);

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, nullptr));
  assert(loader.path);
  assert(strcmp(loader.path, fixture_path) == 0);
  assert(loader.handle);
  assert(loader.setup);

  config_loader_cleanup(&loader);
}

static void test_config_loader_uses_xdg_config_home(const char *fixture_path) {
  config_test_clear_env();

  char temp_root[PATH_MAX]   = {0};
  char config_dir[PATH_MAX]  = {0};
  char config_path[PATH_MAX] = {0};

  config_test_make_temp_dir(
    temp_root,
    sizeof(temp_root),
    "zdwm-config-loader-xdg"
  );
  config_test_join_path(config_dir, sizeof(config_dir), temp_root, "zdwm");
  config_test_join_path(
    config_path,
    sizeof(config_path),
    config_dir,
    "config.so"
  );
  config_test_mkdir(config_dir);
  config_test_symlink(fixture_path, config_path);
  assert(setenv("XDG_CONFIG_HOME", temp_root, 1) == 0);

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, nullptr));
  assert(loader.path);
  assert(strcmp(loader.path, config_path) == 0);
  assert(loader.handle);
  assert(loader.setup);

  config_loader_cleanup(&loader);
  config_test_unlink(config_path);
  config_test_rmdir(config_dir);
  config_test_rmdir(temp_root);
}

static void test_config_loader_uses_home_fallback(const char *fixture_path) {
  config_test_clear_env();

  char temp_root[PATH_MAX]   = {0};
  char config_root[PATH_MAX] = {0};
  char zdwm_dir[PATH_MAX]    = {0};
  char config_path[PATH_MAX] = {0};

  config_test_make_temp_dir(
    temp_root,
    sizeof(temp_root),
    "zdwm-config-loader-home"
  );
  config_test_join_path(config_root, sizeof(config_root), temp_root, ".config");
  config_test_join_path(zdwm_dir, sizeof(zdwm_dir), config_root, "zdwm");
  config_test_join_path(
    config_path,
    sizeof(config_path),
    zdwm_dir,
    "config.so"
  );
  config_test_mkdir(config_root);
  config_test_mkdir(zdwm_dir);
  config_test_symlink(fixture_path, config_path);
  assert(setenv("HOME", temp_root, 1) == 0);

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, nullptr));
  assert(loader.path);
  assert(strcmp(loader.path, config_path) == 0);
  assert(loader.handle);
  assert(loader.setup);

  config_loader_cleanup(&loader);
  config_test_unlink(config_path);
  config_test_rmdir(zdwm_dir);
  config_test_rmdir(config_root);
  config_test_rmdir(temp_root);
}

static void test_config_loader_allows_missing_implicit_library(void) {
  config_test_clear_env();

  char temp_root[PATH_MAX]     = {0};
  char expected_path[PATH_MAX] = {0};

  config_test_make_temp_dir(
    temp_root,
    sizeof(temp_root),
    "zdwm-config-loader-missing"
  );
  config_test_join_path(
    expected_path,
    sizeof(expected_path),
    temp_root,
    "zdwm/config.so"
  );
  assert(setenv("XDG_CONFIG_HOME", temp_root, 1) == 0);

  config_loader_t loader = {0};
  assert(config_loader_load(&loader, nullptr));
  assert(loader.path);
  assert(strcmp(loader.path, expected_path) == 0);
  assert(loader.handle == nullptr);
  assert(loader.setup == nullptr);

  config_loader_cleanup(&loader);
  config_test_rmdir(temp_root);
}

static void test_config_loader_rejects_missing_explicit_library(void) {
  config_test_clear_env();

  config_loader_t loader = {0};
  assert(!config_loader_load(&loader, "/tmp/definitely-missing-config.so"));
  assert(loader.path == nullptr);
  assert(loader.handle == nullptr);
  assert(loader.setup == nullptr);
}

static void test_config_loader_rejects_library_without_setup(
  const char *fixture_path
) {
  config_test_clear_env();

  config_loader_t loader = {0};
  assert(!config_loader_load(&loader, fixture_path));
  assert(loader.path == nullptr);
  assert(loader.handle == nullptr);
  assert(loader.setup == nullptr);
}

int main(int argc, char **argv) {
  assert(argc == 3);

  test_config_loader_loads_explicit_override(argv[1]);
  test_config_loader_override_has_priority_over_env(argv[1]);
  test_config_loader_uses_zdwm_config_lib_env(argv[1]);
  test_config_loader_uses_xdg_config_home(argv[1]);
  test_config_loader_uses_home_fallback(argv[1]);
  test_config_loader_allows_missing_implicit_library();
  test_config_loader_rejects_missing_explicit_library();
  test_config_loader_rejects_library_without_setup(argv[2]);

  return 0;
}
