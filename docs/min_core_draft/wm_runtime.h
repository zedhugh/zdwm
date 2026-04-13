#pragma once

#include <stdint.h>

#include "wm_backend.h"
#include "wm_binding.h"
#include "wm_layout.h"
#include "wm_plan.h"
#include "wm_policy.h"
#include "wm_policy_config.h"
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

typedef struct wm_border_palette_t {
  wm_rgba32_t normal_rgba;
  wm_rgba32_t focused_rgba;
} wm_border_palette_t;

typedef struct wm_runtime_bootstrap_t {
  /*
   * 输出配置（启动时扫描显示器后填充）。
   * 这是 bootstrap 后固定的 output 集合。
   */
  const wm_output_t *outputs;
  size_t output_count;

  /*
   * 工作区配置（已绑定 output）
   * workspace.output_id 指向 outputs 中的某个输出
   * 这是 bootstrap 后固定的 workspace 集合。
   */
  const wm_workspace_t *workspaces;
  size_t workspace_count;

  /*
   * 布局算法注册。
   * layout registry 在 bootstrap 后固定；运行时只切换 workspace.layout_id。
   */
  struct {
    wm_layout_fn fn;
    const char *name;
    const char *symbol;
  } *layouts;
  size_t layout_count;

  /*
   * 策略配置
   */
  wm_policy_config_t policy;

  /*
   * 不可变键盘绑定表。
   * 当前草案把它视为 bootstrap 提供的只读视图；items 的生命周期必须覆盖
   * runtime。
   */
  wm_key_binding_table_t keybindings;

  /*
   * 不可变鼠标按键绑定表。
   * bar / status 点击属于核心外服务，因此这里只覆盖 root / window 目标。
   */
  wm_pointer_binding_table_t pointer_bindings;

  /*
   * 全局统一边框宽度。
   * 它不是窗口真状态。runtime 在提交 configure effect 时：
   * - fullscreen -> 0
   * - 其余模式 -> border_width
   */
  uint16_t border_width;

  /*
   * 全局边框颜色配置。
   * 它不是窗口真状态。runtime 在提交 configure effect 时根据当前焦点关系解析：
   * - 当前 workspace 的 focused window -> focused_rgba
   * - 其余窗口 -> normal_rgba
   */
  wm_border_palette_t border_palette;

  /*
   * Existing windows discovered during startup should be translated into these
   * commands instead of being injected into wm_state_t directly.
   */
  const wm_command_t *initial_commands;
  size_t initial_command_count;
} wm_runtime_bootstrap_t;

// 当前草案直接展示 runtime 结构，便于讨论；实现阶段可再封装
typedef struct wm_runtime_t {
  bool running;

  wm_state_t state;
  wm_plan_t plan;
  wm_command_buffer_t command_buffer;
  wm_layout_registry_t layouts;
  wm_policy_config_t policy;
  wm_key_binding_table_t keybindings;
  wm_pointer_binding_table_t pointer_bindings;
  uint16_t border_width;
  wm_border_palette_t border_palette;
  wm_interaction_state_t interaction;
  wm_backend_t backend;
  wm_service_registry_t services;
} wm_runtime_t;

bool wm_runtime_init(
  wm_runtime_t *runtime,
  wm_backend_t backend,
  const wm_runtime_bootstrap_t *bootstrap
);
bool wm_runtime_register_service(wm_runtime_t *runtime, wm_service_t service);

/*
 * 处理单个事件，方便做无平台依赖的单元测试。
 * 元数据事件允许 runtime 直接更新 wm_window_t 中的元数据字段；
 * 控制状态变更仍经由 route_event() + apply_command()。
 */
bool wm_runtime_process_event(wm_runtime_t *runtime, const wm_event_t *event);

void wm_runtime_run(wm_runtime_t *runtime);
void wm_runtime_stop(wm_runtime_t *runtime);
void wm_runtime_shutdown(wm_runtime_t *runtime);
