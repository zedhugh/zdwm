#pragma once

#include <stddef.h>
#include <stdint.h>

#include "color.h"

typedef struct config_status_t config_status_t;
typedef struct status_t status_t;

typedef enum icon_type_t {
  icon_type_none,
  icon_type_text,
  icon_type_image,
} icon_type_t;

typedef struct renderer_status_item_t {
  icon_type_t icon_type;
  char text[124];
  const char *icon_text;
  const char *icon_path;
  color_t *color;
} renderer_status_item_t;

typedef struct renderer_status_t {
  uint32_t gap;
  uint32_t item_gap;
  size_t item_count;
  renderer_status_item_t item_list[];
} renderer_status_t;

/**
 * @brief 根据 status 配置和实时状态生成可绘制的 status 数据
 *
 * @param config status 配置
 * @param config_count 最大输出项数，传 0 表示使用 config->status_count
 * @param status 实时状态数据
 * @param output 渲染输出缓冲
 */
void renderer_render_status(config_status_t *config, size_t config_count,
                            status_t *status, renderer_status_t *output);
