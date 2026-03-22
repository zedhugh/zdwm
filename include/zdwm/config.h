#ifndef ZDWM_CONFIG_H
#define ZDWM_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/layout.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ZDWM_CONFIG_ABI_VERSION 1u

typedef struct zdwm_config_builder_t zdwm_config_builder_t;

typedef struct zdwm_builtin_layouts_t {
  zdwm_layout_fn tile;
  zdwm_layout_fn monocle;
} zdwm_builtin_layouts_t;

typedef struct zdwm_api_t {
  uint32_t abi_version;
  zdwm_builtin_layouts_t builtin_layouts;

  zdwm_layout_id_t (*register_layout)(zdwm_config_builder_t *builder,
                                      const char *name, const char *symbol,
                                      const char *description,
                                      zdwm_layout_fn fn);

  bool (*define_workspace)(zdwm_config_builder_t *builder, size_t output_index,
                           const char *name, const zdwm_layout_id_t *layout_ids,
                           size_t layout_count,
                           zdwm_layout_id_t initial_layout_id);
} zdwm_api_t;

bool zdwm_config_setup(const zdwm_api_t *api, zdwm_config_builder_t *builder,
                       const zdwm_output_info_t *outputs, size_t output_count);

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_CONFIG_H */
