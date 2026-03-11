#pragma once

#include <stddef.h>

#include "core/types.h"

typedef struct wm_layout_window_ref_t {
  wm_window_id_t window_id;
  wm_window_geometry_mode_t geometry_mode;
} wm_layout_window_ref_t;

typedef struct wm_layout_ctx_t {
  wm_output_id_t output_id;
  wm_workspace_id_t workspace_id;
  wm_window_id_t focused_window_id;
  wm_rect_t workarea;
  const wm_layout_window_ref_t *windows;
  size_t window_count;
} wm_layout_ctx_t;

typedef struct wm_layout_item_t {
  wm_window_id_t window_id;
  wm_rect_t rect; /* 平铺窗口的目标外框矩形（包含边框） */
} wm_layout_item_t;

typedef struct wm_layout_result_t {
  wm_layout_item_t *items;
  size_t item_count;
  size_t item_capacity;
} wm_layout_result_t;

/*
 * 返回 true 表示成功生成合法的布局结果。
 * 返回 false 表示布局计算失败，调用方应丢弃本次结果。
 *
 * 该返回值不表示窗口最终几何是否发生变化；
 * 是否有改动应由调用方在比较当前状态与布局结果后决定。
 */
typedef bool (*wm_layout_fn)(const wm_layout_ctx_t *ctx,
                             wm_layout_result_t *out);

typedef struct wm_layout_slot_t {
  wm_layout_id_t id;
  /*
   * 布局名称，用于：
   * - 配置引用
   * - 日志输出
   * - 调试展示
   *
   * 推荐使用稳定、可读的名称，如 "tile"、"monocle"、"floating"。
   */
  const char *name;
  /*
   * 面向用户的布局说明。
   *
   * 用于帮助信息、调试输出和交互式布局选择；可为空。
   */
  const char *description;
  /*
   * 布局符号，用于状态栏显示等紧凑场景。
   *
   * 推荐使用 1-2 个字符的简短标识符。
   */
  const char *symbol;
  /*
   * 布局执行函数。
   *
   * fn == NULL 表示 floating 布局，即该布局不参与平铺计算。
   */
  wm_layout_fn fn;
} wm_layout_slot_t;

typedef struct wm_layout_registry_t {
  wm_layout_slot_t *slots;
  size_t slot_count;
  size_t slot_capacity;
} wm_layout_registry_t;

void wm_layout_result_init(wm_layout_result_t *result);
void wm_layout_result_reset(wm_layout_result_t *result);
void wm_layout_result_shutdown(wm_layout_result_t *result);

bool wm_layout_result_push(wm_layout_result_t *result, wm_layout_item_t item);

void wm_layout_registry_init(wm_layout_registry_t *registry);
void wm_layout_registry_shutdown(wm_layout_registry_t *registry);
size_t wm_layout_registry_count(const wm_layout_registry_t *registry);
const wm_layout_slot_t *wm_layout_registry_at(
  const wm_layout_registry_t *registry, size_t index);

wm_layout_id_t wm_layout_register(wm_layout_registry_t *registry,
                                  const char *name, const char *symbol,
                                  const char *description, wm_layout_fn fn);
/*
 * 获取可执行的布局函数。
 *
 * 返回 NULL 的情况包括：
 * - id 无效
 * - id 对应的 slot 不存在
 * - slot 存在，但 fn == NULL（表示 floating 布局）
 */
wm_layout_fn wm_layout_get(const wm_layout_registry_t *registry,
                           wm_layout_id_t id);
const wm_layout_slot_t *wm_layout_slot_get(const wm_layout_registry_t *registry,
                                           wm_layout_id_t id);
