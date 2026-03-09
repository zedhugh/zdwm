#pragma once

#include "wm_backend.h"
#include "wm_layout.h"
#include "wm_plan.h"
#include "wm_policy.h"
#include "wm_state.h"

typedef struct wm_runtime_t {
  bool running;

  wm_state_t state;
  wm_plan_t plan;
  wm_command_buffer_t command_buffer;
  wm_layout_registry_t layouts;
  wm_backend_t backend;
} wm_runtime_t;

bool wm_runtime_init(wm_runtime_t *runtime, wm_backend_t backend);

/*
 * 处理单个事件，方便做无平台依赖的单元测试。
 */
bool wm_runtime_process_event(wm_runtime_t *runtime, const wm_event_t *event);

void wm_runtime_run(wm_runtime_t *runtime);
void wm_runtime_stop(wm_runtime_t *runtime);
void wm_runtime_shutdown(wm_runtime_t *runtime);
