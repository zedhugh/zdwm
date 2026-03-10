#pragma once

#include "wm_backend.h"
#include "wm_layout.h"
#include "wm_plan.h"
#include "wm_policy_config.h"
#include "wm_policy.h"
#include "wm_service.h"
#include "wm_state.h"

typedef enum wm_interaction_mode_t {
  WM_INTERACTION_NONE,
  WM_INTERACTION_MOVE_FLOATING,
  WM_INTERACTION_RESIZE_FLOATING,
} wm_interaction_mode_t;

typedef struct wm_interaction_state_t {
  wm_interaction_mode_t mode;
  wm_window_id_t window_id;
  wm_output_id_t origin_output_id;
  wm_point_t pointer_origin;
  wm_rect_t window_origin_rect;
} wm_interaction_state_t;

typedef struct wm_runtime_bootstrap_t {
  /*
   * 输出配置（启动时扫描显示器后填充）
   */
  const wm_output_t *outputs;
  size_t output_count;

  /*
   * 工作区配置（已绑定 output）
   * workspace.output_id 指向 outputs 中的某个输出
   */
  const wm_workspace_t *workspaces;
  size_t workspace_count;

  /*
   * 布局算法注册
   */
  struct {
    wm_layout_fn fn;
    const char *name;
  } *layouts;
  size_t layout_count;

  /*
   * 策略配置
   */
  wm_policy_config_t policy;

  /*
   * Existing windows discovered during startup should be translated into these
   * commands instead of being injected into wm_state_t directly.
   */
  const wm_command_t *initial_commands;
  size_t initial_command_count;
} wm_runtime_bootstrap_t;

// 运行时上下文（实现时采用不透明结构体，此定义仅作参考）
typedef struct wm_runtime_t {
  bool running;

  wm_state_t state;
  wm_plan_t plan;
  wm_command_buffer_t command_buffer;
  wm_layout_registry_t layouts;
  wm_policy_config_t policy;
  wm_interaction_state_t interaction;
  wm_backend_t backend;
  wm_service_registry_t services;
} wm_runtime_t;

bool wm_runtime_init(wm_runtime_t *runtime, wm_backend_t backend,
                     const wm_runtime_bootstrap_t *bootstrap);
bool wm_runtime_register_service(wm_runtime_t *runtime, wm_service_t service);

/*
 * 处理单个事件，方便做无平台依赖的单元测试。
 * 元数据事件允许 runtime 直接更新 auxiliary store；控制状态变更仍经由
 * route_event() + apply_command()。
 */
bool wm_runtime_process_event(wm_runtime_t *runtime, const wm_event_t *event);

void wm_runtime_run(wm_runtime_t *runtime);
void wm_runtime_stop(wm_runtime_t *runtime);
void wm_runtime_shutdown(wm_runtime_t *runtime);
