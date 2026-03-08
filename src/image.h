#pragma once

#include <cairo.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 获取图片按目标高度缩放后的尺寸
 *
 * @param path 图片路径
 * @param target_height 目标高度（像素）
 * @param width 返回缩放后的宽度
 * @param height 返回缩放后的高度
 * @return 加载并计算成功返回 true
 */
bool image_get_scaled_size(const char *path, int32_t target_height, int *width,
                           int *height);

/**
 * @brief 在 cairo 上下文中绘制图片
 *
 * @param cr cairo 上下文
 * @param path 图片路径
 * @param x 绘制起始 x
 * @param y 绘制起始 y
 * @param width 绘制宽度
 * @param height 绘制高度
 * @return 绘制成功返回 true
 */
bool image_draw(cairo_t *cr, const char *path, int32_t x, int32_t y,
                int32_t width, int32_t height);

/**
 * @brief 释放图片缓存
 */
void image_cache_clean(void);
