#pragma once

#include <stddef.h>

#include "core/types.h"
#include "core/window.h"

/* 层级从低到高 */
typedef enum layer_type_t {
  ZDWM_WINDOW_LAYER_DESKTOP = 0,
  ZDWM_WINDOW_LAYER_NORMAL,
  ZDWM_WINDOW_LAYER_TOP,
  ZDWM_WINDOW_LAYER_OVERLAY,
  ZDWM_WINDOW_LAYER_COUNT,
} layer_type_t;

typedef struct layer_stack_t {
  window_id_t *order;
  size_t count;
  size_t capacity;
} layer_stack_t;

/**
 * @brief 释放层栈内部资源并重置为空状态。
 * @param layer 目标层栈。
 */
void layer_stack_cleanup(layer_stack_t *layer);

/**
 * @brief 在该层栈顶部追加一个窗口。
 * @param layer 目标层栈。
 * @param window 待追加的窗口 ID。
 */
void layer_stack_append(layer_stack_t *layer, window_id_t window);

/**
 * @brief 删除指定窗口。
 * @param layer 目标层栈。
 * @param window 待删除的窗口 ID。
 * @return 找到并删除窗口时返回 true，否则返回 false。
 */
bool layer_stack_remove(layer_stack_t *layer, window_id_t window);

/**
 * @brief 将指定窗口提升到该层栈顶。
 * @param layer 目标层栈。
 * @param window 待提升的窗口 ID。
 * @return 仅在窗口存在且顺序发生变化时返回 true，否则返回 false。
 */
bool layer_stack_raise(layer_stack_t *layer, window_id_t window);

/**
 * @brief 将指定窗口下沉到该层栈底。
 * @param layer 目标层栈。
 * @param window 待下沉的窗口 ID。
 * @return 仅在窗口存在且顺序发生变化时返回 true，否则返回 false。
 */
bool layer_stack_lower(layer_stack_t *layer, window_id_t window);

layer_type_t layer_classify(const window_props_t *props);
