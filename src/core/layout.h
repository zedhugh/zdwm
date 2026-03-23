#pragma once

#include <stddef.h>
#include <zdwm/layout.h>

#include "core/types.h"

typedef zdwm_layout_ctx_t layout_ctx_t;
typedef zdwm_layout_item_t layout_item_t;
typedef zdwm_layout_result_t layout_result_t;
typedef zdwm_layout_fn layout_fn;

typedef struct layout_slot_t {
  layout_id_t id;
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
  layout_fn fn;
} layout_slot_t;

typedef struct layout_registry_t {
  layout_slot_t *slots;
  size_t slot_count;
  size_t slot_capacity;
} layout_registry_t;

/*
 * 调用约束：
 * - result 必须是有效的非空指针
 * - 除 init 之外，其余 result 相关接口都要求 result 已初始化
 * - 传入空指针或未初始化对象属于调用方错误
 */
void layout_result_init(layout_result_t *result);
void layout_result_cleanup(layout_result_t *result);

void layout_result_push(layout_result_t *result, layout_item_t item);

/*
 * 调用约束：
 * - registry 必须是有效的非空指针
 * - 除 init 之外，其余 registry 相关接口都要求 registry 已初始化
 * - 传入空指针或未初始化对象属于调用方错误
 */
void layout_registry_init(layout_registry_t *registry);
void layout_registry_cleanup(layout_registry_t *registry);
bool layout_registry_move(layout_registry_t *src, layout_registry_t *dest);
size_t layout_registry_count(const layout_registry_t *registry);
const layout_slot_t *layout_registry_at(const layout_registry_t *registry,
                                        size_t index);

/*
 * 注册一个布局并返回其稳定 ID。
 *
 * 前置条件：
 * - registry 必须已初始化
 *
 * 参数约束：
 * - name 不可为空，用于稳定配置名、日志和调试展示
 * - symbol 不可为空，用于状态栏等紧凑展示
 * - description 可为空
 * - fn 可为空；fn == NULL 表示 floating 布局，不参与平铺计算
 *
 * 返回值：
 * - 成功时返回新注册布局的 ID
 * - name 或 symbol 非法时返回 ZDWM_LAYOUT_ID_INVALID
 *
 * 注册成功后，布局 ID 与其在 registry 中的槽位索引保持一致。
 */
layout_id_t layout_register(layout_registry_t *registry, const char *name,
                            const char *symbol, const char *description,
                            layout_fn fn);
/*
 * 获取可执行的布局函数。
 *
 * 返回 NULL 的情况包括：
 * - id 无效
 * - id 对应的 slot 不存在
 * - slot 存在，但 fn == NULL（表示 floating 布局）
 */
layout_fn layout_get(const layout_registry_t *registry, layout_id_t id);
const layout_slot_t *layout_slot_get(const layout_registry_t *registry,
                                     layout_id_t id);
