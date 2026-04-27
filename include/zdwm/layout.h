#ifndef ZDWM_LAYOUT_H
#define ZDWM_LAYOUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct zdwm_layout_ctx_t {
  zdwm_workspace_id_t workspace_id;
  zdwm_window_id_t focused_window_id;
  zdwm_rect_t output_geometry;
  zdwm_rect_t workarea;
  const zdwm_window_id_t *window_ids;
  size_t window_count;
} zdwm_layout_ctx_t;

typedef struct zdwm_layout_item_t {
  zdwm_window_id_t window_id;
  zdwm_rect_t rect;
} zdwm_layout_item_t;

typedef struct zdwm_layout_result_t {
  zdwm_layout_item_t *items;
  size_t item_count;
  size_t item_capacity;
} zdwm_layout_result_t;

/**
 * @brief 自动布局算法
 *
 * @return out 有变化返回 true 否则返回 true
 */
typedef bool (*zdwm_layout_fn)(
  const zdwm_layout_ctx_t *ctx,
  zdwm_layout_result_t *out
);

static inline void zdwm_layout_result_reset(zdwm_layout_result_t *result) {
  if (!result) abort();
  result->item_count = 0;
}

static inline void
zdwm_layout_result_push(zdwm_layout_result_t *result, zdwm_layout_item_t item) {
  if (!result) abort();

  if (result->item_count == result->item_capacity) {
    size_t capacity = result->item_capacity ? result->item_capacity * 2u : 4u;
    zdwm_layout_item_t *items =
      realloc(result->items, capacity * sizeof(*result->items));
    if (!items) abort();

    result->items         = items;
    result->item_capacity = capacity;
  }

  result->items[result->item_count++] = item;
}

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_LAYOUT_H */
