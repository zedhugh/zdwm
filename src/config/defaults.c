#include "config/defaults.h"

#include "base/macros.h"

bool config_defaults_build(const zdwm_api_t *api,
                           zdwm_config_builder_t *builder,
                           const zdwm_output_info_t *outputs,
                           size_t output_count) {
  if (!api || !builder || !outputs || output_count == 0) return false;

  zdwm_layout_id_t tile_id = api->register_layout(
    builder, "tile", "[]=", "builtin tile", api->builtin_layouts.tile);
  zdwm_layout_id_t monocle_id = api->register_layout(
    builder, "monocle", "[M]", "builtin monocle", api->builtin_layouts.monocle);
  zdwm_layout_id_t floating_id =
    api->register_layout(builder, "floating", "><>", "floating", nullptr);
  if (tile_id == ZDWM_LAYOUT_ID_INVALID ||
      monocle_id == ZDWM_LAYOUT_ID_INVALID ||
      floating_id == ZDWM_LAYOUT_ID_INVALID) {
    return false;
  }

  zdwm_layout_id_t layout_ids[] = {tile_id, monocle_id, floating_id};
  for (size_t i = 0; i < output_count; i++) {
    if (api->define_workspace(builder, i, "main", layout_ids,
                              countof(layout_ids),
                              tile_id) == ZDWM_WORKSPACE_ID_INVALID) {
      return false;
    }
  }

  return true;
}
