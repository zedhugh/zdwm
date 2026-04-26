#ifndef ZDWM_CONFIG_H
#define ZDWM_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zdwm/action.h>
#include <zdwm/layout.h>
#include <zdwm/rules.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ZDWM_CONFIG_ABI_VERSION 1u

typedef struct zdwm_config_builder_t zdwm_config_builder_t;

typedef struct zdwm_builtin_layouts_t {
  zdwm_layout_fn tile;
  zdwm_layout_fn monocle;
} zdwm_builtin_layouts_t;

typedef struct zdwm_api_t {
  uint32_t abi_version;
  zdwm_builtin_layouts_t builtin_layouts;

  /**
   * 用户调用这个函数注册布局算法
   * @param builder 配置构建上下文
   * @param name 布局算法名称，不能为 nullptr
   * @param symbol 布局算法标志符号，用于在状态栏中显示，不能为 nullptr
   * @param description 布局算法描述，可以为 nullptr
   * @param fn 布局算法，可以为 nullptr ，如果为 nullptr 则代表不自动布局
   * @returns 返回布局算法对应的布局 id ，如果注册失败，返回
   * ZDWM_LAYOUT_ID_INVALID
   */
  zdwm_layout_id_t (*register_layout)(
    zdwm_config_builder_t *builder,
    const char *name,
    const char *symbol,
    const char *description,
    zdwm_layout_fn fn
  );

  /**
   * 用户调用这个函数添加 output 中的工作空间
   * @param builder 配置构建上下文
   * @param output_index 该工作空间归属的 output 索引
   * @param name 工作空间名称，用于状态栏显示，不能为 nullptr
   * @param layout_ids 该工作空间支持的布局算法列表
   * @param layout_count 支持的布局算法总数
   * @param initial_layout_id 初始布局算法 id
   * @returns 返回工作空间 id 。如果注册失败，返回 ZDWM_WORKSPACE_ID_INVALID
   */
  zdwm_workspace_id_t (*define_workspace)(
    zdwm_config_builder_t *builder,
    size_t output_index,
    const char *name,
    const zdwm_layout_id_t *layout_ids,
    size_t layout_count,
    zdwm_layout_id_t initial_layout_id
  );

  /**
   * 用户调用这个函数添加窗口规则
   * @param builder 配置构建上下文
   * @param match 窗口匹配条件，不能为 nullptr
   * @param action 匹配后执行的动作，不能为 nullptr
   * @returns 添加成功返回 true ，不合法时返回 false
   * @details 不合法的情况：match 所有字段全为 nullptr；
   *          或 action 无任何动作；或 action 指定的 workspace 不存在
   */
  bool (*add_rule)(
    zdwm_config_builder_t *builder,
    const zdwm_rule_match_t *match,
    const zdwm_rule_action_t *action
  );

  /**
   * @brief 用户调用这个函数添加按键模式
   *
   * @details 按键模式名不能重复，如果添加重复的名字仅第一次成功，后续将添加失败
   *
   * @param builder     配置构建上下文
   * @param mode_name   按键模式名字，不能为 nullptr 或空字符串
   *
   * @return 添加成功返回新增模式的 id 否则返回 ZDWM_BINDING_MODE_ID_INVALID
   */
  zdwm_binding_mode_id_t (*add_mode)(
    zdwm_config_builder_t *builder,
    const char *mode_name
  );

  /**
   * @brief 添加按键绑定
   *
   * @details mode_id 对应的模式不存在或 key_sequence 不合法，都会添加失败
   *
   * @param builder         配置构建上下文
   * @param bind_mode       模式 ID
   * @param key_sequence    按键序列的字符串表达
   * @param action_fn       用户行为函数
   * @param action_arg      用户行为函数所需的参数
   *
   * @return 添加成功返回 true 否则返回 false
   */
  bool (*bind)(
    zdwm_config_builder_t *builder,
    zdwm_binding_mode_id_t bind_mode,
    const char *key_sequence,
    zdwm_action_fn action_fn,
    zdwm_action_arg_t action_arg
  );

  /**
   * @brief 设置默认模式
   *
   * @param table     按键绑定表
   * @param mode_id   按键模式 ID
   *
   * @return 切换成功返回 true 否则返回 false
   */
  bool (*set_default_mode)(
    zdwm_config_builder_t *builder,
    zdwm_binding_mode_id_t mode_id
  );

  /**
   * @brief 设置初始模式
   *
   * @param builder 配置构建上下文
   * @param mode_id 按键模式 ID
   *
   * @return 切换成功返回 true 否则返回 false
   */
  bool (*set_initial_mode)(
    zdwm_config_builder_t *builder,
    zdwm_binding_mode_id_t mode_id
  );
} zdwm_api_t;

/**
 * 用户配置入口函数类型定义
 * @param api 用户自定义时可以使用的 api 接口及数据，api
 * 中的函数和信息只能在函数内部使用，如果用户将其保存后在函数外使用是未定义行为
 * @param builder 配置构建上下文
 * @param outputs 屏幕信息列表
 * @param output_count 屏幕个数
 * @returns 用户配置成功返回 true ，配置失败返回 false
 */
typedef bool zdwm_config_setup_fn(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  const zdwm_output_info_t *outputs,
  size_t output_count
);

/* 用户在自己的动态链接库配置中必须实现该函数 */
extern zdwm_config_setup_fn setup;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_CONFIG_H */
