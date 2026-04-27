#include "config/defaults.h"

#include <zdwm/action.h>
#include <zdwm/types.h>

#include "base/macros.h"

static constexpr char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";

static void spawn(const zdwm_action_ctx_t *ctx, const zdwm_action_arg_t *arg) {
  ctx->spawn(arg->str);
}

bool config_defaults_build(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  const zdwm_output_info_t *outputs,
  size_t output_count
) {
  if (!api || !builder || !outputs || output_count == 0) return false;

  zdwm_layout_id_t fair_id = api->register_layout(
    builder,
    "fair",
    "[]=",
    "builtin fair",
    api->builtin_layouts.fair
  );
  zdwm_layout_id_t maximize_id = api->register_layout(
    builder,
    "maximize",
    "[M]",
    "builtin maximize",
    api->builtin_layouts.maximize
  );
  zdwm_layout_id_t fullscreen_id = api->register_layout(
    builder,
    "fullscreen",
    "[F]",
    "builtin fullscreen",
    api->builtin_layouts.fullscreen
  );
  zdwm_layout_id_t floating_id =
    api->register_layout(builder, "floating", "><>", "floating", nullptr);
  if (fair_id == ZDWM_LAYOUT_ID_INVALID ||
      maximize_id == ZDWM_LAYOUT_ID_INVALID ||
      fullscreen_id == ZDWM_LAYOUT_ID_INVALID ||
      floating_id == ZDWM_LAYOUT_ID_INVALID) {
    return false;
  }

  zdwm_layout_id_t layout_ids[] = {
    fair_id,
    maximize_id,
    fullscreen_id,
    floating_id,
  };
  for (size_t i = 0; i < output_count; i++) {
    if (api->define_workspace(
          builder,
          i,
          "main",
          layout_ids,
          countof(layout_ids),
          fair_id
        ) == ZDWM_WORKSPACE_ID_INVALID) {
      return false;
    }
  }

  auto default_mode = api->add_mode(builder, "default");

#define BIND(mode, key, fn, arg) \
  api->bind(builder, mode, key, fn, (zdwm_action_arg_t)arg)

  BIND(default_mode, "Mod4+r", spawn, {.str = launcher});

#undef BIND

  api->set_default_mode(builder, default_mode);
  api->set_initial_mode(builder, default_mode);

  return true;
}
