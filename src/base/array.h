#pragma once

#include <stddef.h>
#include <string.h>

#include "base/memory.h"

/**
 * @file array.h
 * @brief 动态数组基础操作。
 */

/**
 * @brief 确保数组容量至少为 need。
 *
 * @param items 数组指针地址
 * @param item_size 单个元素大小
 * @param capacity 当前容量地址
 * @param need 目标容量
 */
static inline void array_reserve_impl(void **items, size_t item_size,
                                      size_t *capacity, size_t need) {
  if (*capacity >= need) return;

  size_t new_capacity = next_capacity(*capacity);
  while (new_capacity < need) {
    new_capacity = next_capacity(new_capacity);
  }

  if (*items) {
    xrealloc(items, item_size * new_capacity);
  } else {
    *items = xmalloc(item_size * new_capacity);
  }
  *capacity = new_capacity;
}

/**
 * @brief 追加一个槽位并返回其地址。
 *
 * @param items 数组指针地址
 * @param item_size 单个元素大小
 * @param count 元素个数地址
 * @param capacity 容量地址
 *
 * @return void* 新槽位地址
 */
static inline void *array_push_impl(void **items, size_t item_size,
                                    size_t *count, size_t *capacity) {
  array_reserve_impl(items, item_size, capacity, *count + 1);

  void *slot = (char *)*items + *count * item_size;
  (*count)++;
  return slot;
}

/**
 * @brief 删除指定下标元素，并将后续元素左移。
 *
 * @param items 数组首地址
 * @param item_size 单个元素大小
 * @param count 元素个数地址
 * @param index 要删除的下标
 *
 * @return true 删除成功
 * @return false 下标越界
 */
static inline bool array_erase_impl(void *items, size_t item_size,
                                    size_t *count, size_t index) {
  if (index >= *count) return false;

  size_t tail_count = *count - index - 1;
  if (tail_count > 0) {
    char *dest = (char *)items + index * item_size;
    char *src = dest + item_size;
    memmove(dest, src, tail_count * item_size);
  }

  (*count)--;
  return true;
}

/** @brief 为具体类型数组预留容量。 */
#define array_reserve(items, capacity, need) \
  array_reserve_impl((void **)&(items), sizeof(*(items)), &(capacity), (need))

/** @brief 为具体类型数组追加一个槽位。 */
#define array_push(items, count, capacity) \
  array_push_impl((void **)&(items), sizeof(*(items)), &(count), &(capacity))

/** @brief 删除具体类型数组中指定下标的元素。 */
#define array_erase(items, count, index) \
  array_erase_impl((items), sizeof(*(items)), &(count), (index))
