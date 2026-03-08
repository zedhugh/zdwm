#pragma once

#include <stddef.h>
#include <stdint.h>

#include "renderer.h"
#include "status.h"

typedef struct config_status_item_t {
  status_type_t type;
  icon_type_t icon_type;
  const char *icon;
  const char *color;
} config_status_item_t;

typedef struct config_status_t {
  uint32_t status_gap;
  uint32_t status_item_gap;
  size_t status_count;
  config_status_item_t list[];
} config_status_t;

void config_status_set_gap(config_status_t *status_config, uint32_t gap,
                           uint32_t item_gap);

void config_status_add_item(config_status_t **status_config,
                            config_status_item_t status_item);

typedef bool (*status_item_filter)(config_status_item_t *item);
/**
 * @brief 按过滤条件原地筛选 status 项并保持原有顺序
 *
 * 当 filter(item) 返回 true 时保留该项，返回 false 时删除该项。
 * 筛选后会压缩 list、更新 status_count，并收缩配置对象内存。
 *
 * @param status_config 配置指针地址（函数内部会重分配内存）
 * @param filter 过滤函数
 */
void config_status_filter_item(config_status_t **status_config,
                               status_item_filter filter);

typedef void (*status_renderer_t)(config_status_t *config, size_t config_count,
                                  status_t *status, renderer_status_t *output);

typedef struct config_t {
  config_status_t *status;
  status_renderer_t status_renderer;
} config_t;

/**
 * @brief 初始化配置对象
 * @return 返回静态变量指针，不需要释放
 */
const config_t *init_config(void);

void config_set_custom_status_renderer(config_t *config,
                                       status_renderer_t status_renderer);
