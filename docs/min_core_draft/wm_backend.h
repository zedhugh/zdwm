#pragma once

#include "wm_binding.h"
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
   * 安装 bootstrap 固定下来的键盘绑定表。
   * X11 这类需要被动抓键的平台可以在这里建立平台侧 grab；
   * 总能收到所有键盘事件的 backend 可以把它留空。
   */
  bool (*set_keybindings)(wm_backend_t *backend,
                          const wm_key_binding_table_t *keybindings);
  /*
   * 安装 bootstrap 固定下来的鼠标按键绑定表。
   * X11 这类需要被动抓按钮的平台可以把它缓存下来，并在 root 或新受管窗口
   * 上按 target 规则建立 grab；总能收到所有 pointer button 事件的 backend
   * 可以把它留空。
   */
  bool (*set_pointer_bindings)(
      wm_backend_t *backend,
      const wm_pointer_binding_table_t *pointer_bindings);
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
