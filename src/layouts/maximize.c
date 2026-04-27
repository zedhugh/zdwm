#include "layouts/maximize.h"

#include <zdwm/layout.h>

bool maximize(const zdwm_layout_ctx_t *ctx, zdwm_layout_result_t *out) {
  auto windows = ctx->window_ids;
  auto count   = ctx->window_count;
  if (!windows || !count) return false;

  for (size_t i = 0; i < count; ++i) {
    zdwm_layout_item_t item = {
      .window_id = windows[i],
      .rect      = ctx->workarea,
    };
    zdwm_layout_result_push(out, item);
  }

  return true;
}
