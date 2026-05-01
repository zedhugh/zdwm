#pragma once

#include <stdint.h>

typedef struct color_t {
  uint32_t rgba;
  uint32_t argb;

  /* RGBA color channels for Cairo, each channel in the range [0, 1] */
  double red, green, blue, alpha;
} color_t;

/**
 * @brief 解析 16 进制表示的颜色
 *
 * @param hex   颜色的 16 进制表示，支持的格式有
 *              RGB/RGBA/RRGGBB/RRGGBBAA/#RGB/#RGBA/#RRGGBB/#RRGGBBAA
 * @param color 存储颜色值的结构体指针，不能为 nullptr
 */
void color_parse(const char *hex, color_t *const color);
