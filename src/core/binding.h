#pragma once

#include <stddef.h>
#include <zdwm/action.h>
#include <zdwm/types.h>

#include "core/types.h"

typedef struct binding_table_t binding_table_t;

typedef struct key_binding_t {
  const char *key_str;
  modifier_mask_t modifiers;
  keysym_t keysym;
  zdwm_action_fn *fn;
  zdwm_action_arg_t *arg;
} key_binding_t;

/**
 * @brief 添加新的模式
 *
 * @details 按键模式名不能重复，如果添加重复的名字仅第一次成功，后续将添加失败
 *
 * @param table     按键绑定表
 * @param mode_name 按键模式名字，不能为 nullptr 或空字符串
 *
 * @return 添加成功返回新增模式的 id 否则返回 ZDWM_BINDING_MODE_ID_INVALID
 */
zdwm_binding_mode_id_t
binding_table_add_mode(binding_table_t *table, const char *mode_name);

/**
 * @brief 添加按键绑定
 *
 * @details mode_id 对应的模式不存在或 key_sequence 不合法，都会添加失败
 *
 * @param table         按键绑定表
 * @param mode_id       模式 ID
 * @param key_sequence  按键序列的字符串表达
 * @param fn            用户行为函数
 * @param arg           用户行为函数所需的参数
 *
 * @return 添加成功返回 true 否则返回 false
 */
bool binding_table_add_bind(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id,
  const char *key_sequence,
  zdwm_action_fn fn,
  zdwm_action_arg_t *arg
);

/**
 * @brief 销毁按键绑定表
 */
void binding_table_destroy(binding_table_t *table);

/**
 * @brief 创建按键绑定表
 */
binding_table_t *binding_table_create(void);

/**
 * @brief 通过模式 ID 设置默认模式
 *
 * @param table     按键绑定表
 * @param mode_id   按键模式 ID
 *
 * @return 切换成功返回 true 否则返回 false
 */
bool binding_table_set_default_mode(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id
);

/**
 * @brief 通过模式 ID 设置当前模式
 *
 * @param table     按键绑定表
 * @param mode_id   按键模式 ID
 *
 * @return 切换成功返回 true 否则返回 false
 */
bool binding_table_set_current_mode(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id
);

/**
 * @brief 切换按键绑定模式
 *
 * @param table 按键绑定表
 * @param delta 切换变化量
 *
 * @return 无法切换返回 false 否则返回 true
 */
bool binding_table_cycle_mode(binding_table_t *table, int delta);

/**
 * @brief 获取当前按键绑定列表
 *
 * @param table 按键绑定表
 * @param count 保存按键个数的指针
 *
 * @return 返回当前按键列表的头指针，调用方仅只读访问，不持有对应的内存。
 *
 * @notice
 * 1. count 指针不能为 nullptr ，若 count 传 nullptr 视为调用方错误
 * 2. 调用方需判断返回的指针是否为 nullptr
 */
const key_binding_t *
binding_table_get_current_bindings(binding_table_t *table, size_t *count);
