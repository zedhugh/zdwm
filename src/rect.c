#include "rect.h"

#include "base/macros.h"
#include "base/memory.h"

static inline bool rect_valid(rect_t rect) {
  return rect.x1 < rect.x2 && rect.y1 < rect.y2;
}

/* 将一个合法矩形追加到动态矩形数组中。 */
static inline void rect_list_append(rect_t **list, size_t *length,
                                    size_t *capacity, rect_t rect) {
  if (!rect_valid(rect)) return;

  if (*length == *capacity) {
    *capacity = *capacity ? *capacity * 2 : 4;
    p_realloc(list, *capacity);
  }
  (*list)[(*length)++] = rect;
}

bool rect_intersection(rect_t a, rect_t b, rect_t *intersection) {
  if (!rect_valid(a) || !rect_valid(b)) return false;

  rect_t overlap = {
    .x1 = MAX(a.x1, b.x1),
    .y1 = MAX(a.y1, b.y1),
    .x2 = MIN(a.x2, b.x2),
    .y2 = MIN(a.y2, b.y2),
  };
  if (!rect_valid(overlap)) return false;

  if (intersection) *intersection = overlap;
  return true;
}

size_t rect_subtract(rect_t source, rect_t clip, rect_t remaining[4]) {
  if (!rect_valid(source)) return 0;

  rect_t overlap;
  if (!rect_intersection(source, clip, &overlap)) {
    if (remaining) remaining[0] = source;
    return 1;
  }

  size_t count = 0;
  /*
   * source:  [sx1,sx2) x [sy1,sy2)
   * overlap: [ox1,ox2) x [oy1,oy2) = source 与 clip 的交集
   *
   * y=sy1   ┌───────────────────────────────────────────┐
   *         │                     TOP                   │
   *         │      UL                             UR    │
   * y=oy1   ├───────────────┬───────────────┬───────────┤
   *         │     LEFT      │    OVERLAP    │   RIGHT   │
   * y=oy2   ├───────────────┴───────────────┴───────────┤
   *         │      LL                             LR    │
   *         │                    BOTTOM                 │
   * y=sy2   └───────────────────────────────────────────┘
   *        x=sx1           x=ox1           x=ox2       x=sx2
   *
   * 角块归属：
   * UL、UR 属于 TOP
   * LL、LR 属于 BOTTOM
   * LEFT/RIGHT 只覆盖 y ∈ [oy1, oy2)，因此不会包含四个角块。
   */
  rect_t parts[] = {
    /* top */
    {
      .x1 = source.x1,
      .y1 = source.y1,
      .x2 = source.x2,
      .y2 = overlap.y1,
    },
    /* bottom */
    {
      .x1 = source.x1,
      .y1 = overlap.y2,
      .x2 = source.x2,
      .y2 = source.y2,
    },
    /* left */
    {
      .x1 = source.x1,
      .y1 = overlap.y1,
      .x2 = overlap.x1,
      .y2 = overlap.y2,
    },
    /* right */
    {
      .x1 = overlap.x2,
      .y1 = overlap.y1,
      .x2 = source.x2,
      .y2 = overlap.y2,
    },
  };

  for (size_t i = 0; i < countof(parts); i++) {
    if (!rect_valid(parts[i])) continue;
    if (remaining) remaining[count] = parts[i];
    count++;
  }

  return count;
}

size_t rect_subtract_many(rect_t source, const rect_t clips[],
                          size_t clip_count, rect_t **remaining) {
  /*
   * 实现说明：
   * 初始剩余集合为 {source}，然后依次处理每个 clip：
   * 1) 用 clip 去减当前集合中的每个碎片
   * 2) 将产生的新碎片合并到 next 集合
   * 3) 用 next 替换 current，进入下一轮
   * 最终 current 即 source - clips[0] - clips[1] - ... 的结果。
   */
  if (remaining) *remaining = nullptr;
  if (!rect_valid(source)) return 0;

  size_t current_length = 1;
  size_t current_capacity = 1;
  rect_t *current = p_new(rect_t, current_capacity);
  current[0] = source;

  if (clips) {
    for (size_t i = 0; i < clip_count && current_length > 0; i++) {
      rect_t clip = clips[i];
      if (!rect_valid(clip)) continue;

      size_t next_length = 0;
      size_t next_capacity = current_length ? current_length : 1;
      rect_t *next = p_new(rect_t, next_capacity);

      for (size_t j = 0; j < current_length; j++) {
        rect_t fragments[4];
        size_t fragment_count = rect_subtract(current[j], clip, fragments);
        for (size_t k = 0; k < fragment_count; k++) {
          rect_list_append(&next, &next_length, &next_capacity, fragments[k]);
        }
      }

      p_delete(&current);
      current = next;
      current_length = next_length;
      current_capacity = next_capacity;
    }
  }

  if (remaining) {
    *remaining = current;
  } else {
    p_delete(&current);
  }

  return current_length;
}
