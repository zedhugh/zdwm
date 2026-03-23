#include <zdwm/config.h>

static bool fixture_layout(const zdwm_layout_ctx_t *ctx,
                           zdwm_layout_result_t *out) {
  if (!ctx || !out) return false;

  zdwm_layout_result_reset(out);
  if (!ctx->window_ids || ctx->window_count == 0) return true;

  zdwm_layout_result_push(out, (zdwm_layout_item_t){
                                 .window_id = ctx->window_ids[0],
                                 .rect = ctx->workarea,
                               });
  return true;
}

bool setup(const zdwm_api_t *api, zdwm_config_builder_t *builder,
           const zdwm_output_info_t *outputs, size_t output_count) {
  if (!api || !builder || !outputs || output_count == 0) return false;

  zdwm_layout_id_t layout_id =
    api->register_layout(builder, "test-only", "[T]", "fixture layout",
                         fixture_layout);
  if (layout_id == ZDWM_LAYOUT_ID_INVALID) return false;

  return api->define_workspace(builder, 0, "from-so", &layout_id, 1,
                               layout_id) != ZDWM_WORKSPACE_ID_INVALID;
}
