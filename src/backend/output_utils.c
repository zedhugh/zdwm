#include "backend/output_utils.h"

#include <stddef.h>
#include <stdint.h>

#include "core/backend.h"
#include "core/types.h"
#include "core/wm_desc.h"
#include "utils.h"

static inline int32_t right(wm_rect_t a) { return a.x + a.width; }
static inline int32_t bottom(wm_rect_t a) { return a.y + a.height; }
static inline bool fully_contains(wm_rect_t outer, wm_rect_t inner) {
  return inner.x >= outer.x && inner.y >= outer.y &&
         right(inner) <= right(outer) && bottom(inner) <= bottom(outer);
}

wm_backend_detect_t *output_remove_duplication(const wm_output_info_t *output,
                                               const size_t count) {
  if (!output || count < 1) return nullptr;

  bool *remove = p_new(bool, count);

  for (int32_t i = (int32_t)count - 1; i >= 0; i--) {
    if (remove[i]) continue;
    for (int32_t j = i - 1; j >= 0; j--) {
      if (remove[j]) continue;
      if (fully_contains(output[j].geometry, output[i].geometry)) {
        remove[i] = true;
      } else if (fully_contains(output[i].geometry, output[j].geometry)) {
        remove[j] = true;
      }
    }
  }

  int32_t amount = 0;
  for (size_t i = 0; i < count; i++) {
    if (!remove[i]) amount++;
  }
  if (!amount) {
    p_delete(&remove);
    return nullptr;
  }

  wm_output_info_t *list = p_new(wm_output_info_t, amount);
  amount = 0;
  for (size_t i = 0; i < count; i++) {
    if (remove[i]) continue;
    list[amount++] = output[i];
  }
  p_delete(&remove);
  wm_backend_detect_t *detect = p_new(wm_backend_detect_t, 1);
  detect->outputs = list;
  detect->output_count = amount;
  return detect;
}
