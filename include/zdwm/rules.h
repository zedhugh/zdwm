#ifndef ZDWM_RULES_H
#define ZDWM_RULES_H

#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief 窗口匹配条件
 * @details 仅匹配不为 nullptr 的字段，多个字段按且规则匹配
 */
typedef struct zdwm_rule_match_t {
  const char *app_id;
  const char *role;
  const char *class_name;
  const char *instance_name;
} zdwm_rule_match_t;

typedef struct zdwm_rule_action_t {
  /**
   * 目标 workspace id
   * - 若规则不想指定 workspace ，则将其设为 ZDWM_WORKSPACE_ID_INVALID
   */
  zdwm_workspace_id_t workspace;

  bool switch_to_workspace; /* 自动切换到目标 workspace（仅 true 时生效） */
  bool fullscreen;          /* 强制全屏（仅 true 时生效） */
  bool maximize;            /* 强制最大化（仅 true 时生效） */
  bool floating;            /* 强制浮动（仅 true 时生效） */
} zdwm_rule_action_t;

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_RULES_H */
