#pragma once

#include "wm_event.h"
#include "wm_plan.h"

typedef struct wm_backend_t wm_backend_t;

typedef enum wm_backend_next_result_t {
  // 成功产出一个 runtime 输入事件，out 有效
  WM_BACKEND_NEXT_EVENT,
  // backend 请求 runtime 正常停止（例如连接关闭）
  WM_BACKEND_NEXT_STOP,
  // backend 检测到必须重建 bootstrap 的平台变化（例如显示器配置变化）
  WM_BACKEND_NEXT_RESTART_REQUIRED,
  // backend 遇到无法恢复的错误
  WM_BACKEND_NEXT_ERROR,
} wm_backend_next_result_t;

typedef struct wm_backend_api_t {
  bool (*init)(wm_backend_t *backend);
  /*
   * 返回一个 backend 轮询结果。
   * 只有返回 WM_BACKEND_NEXT_EVENT 时，out 才包含一个完整的 runtime 输入事件。
   */
  wm_backend_next_result_t (*next_event)(wm_backend_t *backend, wm_event_t *out);
  /*
   * Runtime filters out service-side effects such as WM_EFFECT_RENDER_OUTPUT.
   * Backends only receive platform effects here.
   */
  bool (*apply_effect)(wm_backend_t *backend, const wm_effect_t *effect);
  void (*flush)(wm_backend_t *backend);
  void (*shutdown)(wm_backend_t *backend);
} wm_backend_api_t;

struct wm_backend_t {
  const wm_backend_api_t *api;
  void *impl;
};
