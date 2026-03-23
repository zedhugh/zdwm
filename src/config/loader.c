#include "config/loader.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>

#include "base/log.h"
#include "base/memory.h"

static char *config_loader_join_path(const char *base, const char *suffix) {
  if (!base || !suffix) return nullptr;

  size_t base_len = strlen(base);
  size_t suffix_len = strlen(suffix);
  char *path = p_new(char, base_len + suffix_len + 1);
  memcpy(path, base, base_len);
  memcpy(path + base_len, suffix, suffix_len + 1);
  return path;
}

static bool config_loader_resolve_path(char **out_path, bool *out_is_explicit,
                                       const char *override_path) {
  const char *path = override_path;
  if (path && *path) {
    *out_path = p_strdup(path);
    *out_is_explicit = true;
    return true;
  }

  path = getenv("ZDWM_CONFIG_LIB");
  if (path && *path) {
    *out_path = p_strdup(path);
    *out_is_explicit = true;
    return true;
  }

  path = getenv("XDG_CONFIG_HOME");
  if (path && *path) {
    *out_path = config_loader_join_path(path, "/zdwm/config.so");
    *out_is_explicit = false;
    return *out_path != nullptr;
  }

  path = getenv("HOME");
  if (path && *path) {
    *out_path = config_loader_join_path(path, "/.config/zdwm/config.so");
    *out_is_explicit = false;
    return *out_path != nullptr;
  }

  return false;
}

bool config_loader_load(config_loader_t *loader, const char *override_path) {
  if (!loader) return false;
  config_loader_cleanup(loader);

  char *path = nullptr;
  bool is_explicit = false;
  if (!config_loader_resolve_path(&path, &is_explicit, override_path)) {
    return false;
  }

  loader->path = path;

  if (access(path, F_OK) != 0) {
    if (is_explicit) {
      warn("config library not found: %s", path);
      config_loader_cleanup(loader);
      return false;
    }
    return true;
  }

  loader->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!loader->handle) {
    warn("failed to load config library %s: %s", path, dlerror());
    config_loader_cleanup(loader);
    return false;
  }

  dlerror();
  loader->setup = (zdwm_config_setup_fn *)dlsym(loader->handle, "setup");
  const char *error = dlerror();
  if (error) {
    warn("failed to resolve setup from %s: %s", path, error);
    config_loader_cleanup(loader);
    return false;
  }

  return true;
}

void config_loader_cleanup(config_loader_t *loader) {
  if (!loader) return;

  loader->setup = nullptr;

  if (loader->handle) dlclose(loader->handle);
  loader->handle = nullptr;

  p_delete(&loader->path);
}
