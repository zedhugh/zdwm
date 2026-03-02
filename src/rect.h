#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct rect_t {
  int32_t x1;
  int32_t y1;
  int32_t x2;
  int32_t y2;
} rect_t;

bool rect_intersection(rect_t a, rect_t b, rect_t *intersection);
size_t rect_subtract(rect_t source, rect_t clip, rect_t remaining[4]);
/**
 * @brief 从 source 中按顺序减去 clips 中的多个矩形
 * @param source 源矩形
 * @param clips 要减去的矩形数组，可为 nullptr
 * @param clip_count clips 的长度
 * @param remaining 输出剩余矩形数组；若不为 nullptr，则由函数分配内存并由调用方释放
 * @return 剩余矩形数量
 *
 * @code{.c}
 * rect_t source = {.x1 = 0, .y1 = 0, .x2 = 100, .y2 = 80};
 * rect_t clips[] = {
 *   {.x1 = 10, .y1 = 10, .x2 = 40, .y2 = 40},
 *   {.x1 = 30, .y1 = 20, .x2 = 70, .y2 = 60},
 * };
 *
 * rect_t *remaining = nullptr;
 * size_t count = rect_subtract_many(source, clips, 2, &remaining);
 *
 * for (size_t i = 0; i < count; i++) {
 *   // use remaining[i]
 * }
 *
 * free(remaining);
 * @endcode
 */
size_t rect_subtract_many(rect_t source, const rect_t clips[],
                          size_t clip_count, rect_t **remaining);
