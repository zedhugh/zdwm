#include "layouts/fair.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/layout.h>

bool fair(const zdwm_layout_ctx_t *ctx, zdwm_layout_result_t *out) {
  auto windows = ctx->window_ids;
  auto count   = (int32_t)ctx->window_count;
  if (!windows || !count) return false;

  zdwm_layout_result_reset(out);

  auto columns = (int32_t)floor(sqrt(count));
  if (columns * (columns + 1) <= count) ++columns;

  auto rows_in_other_cols = count / columns;
  auto rows_in_main_col   = count - (columns - 1) * rows_in_other_cols;
  while (rows_in_main_col > rows_in_other_cols) {
    ++rows_in_other_cols;
    rows_in_main_col = count - (columns - 1) * rows_in_other_cols;
  }

  auto workarea            = &ctx->workarea;
  auto width_avg           = workarea->width / columns;
  auto width_for_main_col  = workarea->width - (columns - 1) * width_avg;
  auto width_for_other_col = width_avg;

  int32_t row = 0, col = 0, row_count;
  int32_t x = workarea->x, y = workarea->y;
  for (int32_t i = 0; i < count; ++i) {
    int32_t width;

    if (i < rows_in_main_col) {
      row       = i;
      width     = width_for_main_col;
      row_count = rows_in_main_col;
    } else {
      width     = width_for_other_col;
      row_count = rows_in_other_cols;
      if (((i - rows_in_main_col) % row_count) == 0) {
        col++;
        row  = 0;
        x   += col == 1 ? width_for_main_col : width_for_other_col;
        y    = workarea->y;
      }
    }

    auto h_avg              = workarea->height / row_count;
    auto h_main             = workarea->height - h_avg * (row_count - 1);
    auto height             = row == 0 ? h_main : h_avg;
    zdwm_layout_item_t item = {
      .window_id = windows[i],
      .rect      = {.x = x, .y = y, .width = width, .height = height}
    };
    zdwm_layout_result_push(out, item);

    y += height;
  }

  return true;
}
