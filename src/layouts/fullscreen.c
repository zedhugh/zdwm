#include "layouts/fullscreen.h"

bool fullscreen(const zdwm_layout_ctx_t *ctx, zdwm_layout_result_t *out) {
  auto windows = ctx->window_ids;
  auto count   = ctx->window_count;
  if (!windows || !count) return false;

  zdwm_layout_result_reset(out);
  for (size_t i = 0; i < count; ++i) {
    zdwm_layout_item_t item = {
      .window_id = windows[i],
      .rect      = ctx->output_geometry,
    };
    zdwm_layout_result_push(out, item);
  }

  return true;
}
