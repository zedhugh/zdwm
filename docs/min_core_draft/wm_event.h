#pragma once

#include "wm_desc.h"

/*
 * wm_event_t 表示“runtime 真正关心的输入事实”：
 * - 它不是平台原始事件的逐字段照抄
 * - 它允许 backend 对平台事件做最小必要的归一化 / 过滤 / 聚合
 * - 它不承载 runtime 控制语义；stop / restart / error 由 backend.next_event()
 *   的返回结果表达，而不是伪装成一个事件类型
 */
typedef enum wm_event_type_t {
  // 最小核心里的快捷键只由 key press 触发，不单独暴露 key release 事件
  WM_EVENT_KEY_PRESS,
  // 鼠标绑定只匹配 button press；button release 主要供交互态结束拖拽使用
  WM_EVENT_POINTER_BUTTON_PRESS,
  WM_EVENT_POINTER_BUTTON_RELEASE,
  WM_EVENT_POINTER_MOTION,
  // 指针焦点进入某个窗口；对应 X11 EnterNotify / Wayland wl_pointer.enter
  WM_EVENT_POINTER_ENTER,
  WM_EVENT_WINDOW_MAP_REQUEST,
  WM_EVENT_WINDOW_REMOVE,
  WM_EVENT_WINDOW_METADATA_CHANGED,
  WM_EVENT_WINDOW_HINTS_CHANGED,
  WM_EVENT_WINDOW_ACTIVATE_REQUEST,
  WM_EVENT_WINDOW_STATE_REQUEST,
  WM_EVENT_CONFIGURE_REQUEST,
} wm_event_type_t;

typedef struct wm_key_press_event_t {
  /*
   * 规范化后的逻辑主键编码（xkb keysym）。
   * backend 应先结合 keymap / xkb state 把原始按键解析成逻辑键，再写入这里。
   * 它适合与 "SUPER+m" 这类配置字符串解析结果做匹配。
   */
  wm_keysym_t keysym;
  // backend 归一化后的修饰键掩码
  wm_modifier_mask_t modifiers;
  /*
   * backend 原始物理键编码。
   * 它保留平台侧按键信息，便于调试、日志或未来按物理键位绑定；
   * 常规快捷键匹配不应直接使用它。
   */
  uint32_t keycode;
} wm_key_press_event_t;

typedef struct wm_pointer_button_event_t {
  wm_button_t button;
  wm_modifier_mask_t modifiers;
  // backend 原始按钮码，仅供调试或未来平台特化逻辑使用
  uint32_t raw_button;
  // 全局桌面坐标
  wm_point_t root;
  // 相对 window 的坐标；若 window == WM_WINDOW_ID_INVALID，则该值未定义
  wm_point_t local;
  // 当前命中的窗口；若未命中窗口，则为 WM_WINDOW_ID_INVALID
  wm_window_id_t window;
} wm_pointer_button_event_t;

typedef struct wm_pointer_motion_event_t {
  // 全局桌面坐标
  wm_point_t root;
  // 相对 window 的坐标；若 window == WM_WINDOW_ID_INVALID，则该值未定义
  wm_point_t local;
  // 当前命中的窗口；若未命中窗口，则为 WM_WINDOW_ID_INVALID
  wm_window_id_t window;
} wm_pointer_motion_event_t;

typedef struct wm_pointer_enter_event_t {
  // 全局桌面坐标
  wm_point_t root;
  // 相对 window 的坐标；若 window == WM_WINDOW_ID_INVALID，则该值未定义
  wm_point_t local;
  // 当前进入并获得 pointer focus 的窗口；若未命中窗口，则为
  // WM_WINDOW_ID_INVALID
  wm_window_id_t window;
} wm_pointer_enter_event_t;

typedef struct wm_window_map_request_event_t {
  wm_window_info_t info;
  // 仅用于 manage 路由判断，不进入持久 state
  wm_window_id_t transient_for;
  // 仅用于 manage 路由判断，不进入持久 state
  bool is_dialog;
} wm_window_map_request_event_t;

/*
 * 归一化后的窗口移除事件。
 * backend 把 withdraw / destroy 这类平台差异折叠成单一 remove 事件，
 * core 只按 reason 区分“为何离开 managed 集合”。
 */
typedef enum wm_window_remove_reason_t {
  WM_WINDOW_REMOVE_WITHDRAWN,
  WM_WINDOW_REMOVE_DESTROY,
} wm_window_remove_reason_t;

typedef struct wm_window_remove_event_t {
  wm_window_id_t window;
  wm_window_remove_reason_t reason;
} wm_window_remove_event_t;

typedef enum wm_window_meta_changed_flags_t {
  WM_WINDOW_META_CHANGED_NONE     = 0,
  WM_WINDOW_META_CHANGED_TITLE    = 1u << 0,
  WM_WINDOW_META_CHANGED_APP_ID   = 1u << 1,
  WM_WINDOW_META_CHANGED_CLASS    = 1u << 2,
  WM_WINDOW_META_CHANGED_INSTANCE = 1u << 3,
} wm_window_meta_changed_flags_t;

typedef struct wm_window_metadata_changed_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  /*
   * These strings are borrowed from the backend adapter and are valid until
   * wm_runtime_process_event() returns. changed_fields decides which ones are
   * meaningful for the current event.
   * runtime 会复制这些字符串到 wm_window_t 中的对应字段。
   */
  const char *title;
  const char *app_id;
  const char *class_name;
  const char *instance_name;
} wm_window_metadata_changed_event_t;

typedef enum wm_window_hint_changed_flags_t {
  WM_WINDOW_HINT_CHANGED_NONE         = 0,
  WM_WINDOW_HINT_CHANGED_URGENT       = 1u << 0,
  WM_WINDOW_HINT_CHANGED_FIXED_SIZE   = 1u << 1,
  WM_WINDOW_HINT_CHANGED_SKIP_TASKBAR = 1u << 2,
} wm_window_hint_changed_flags_t;

/*
 * 客户端属性 / hint 变化的归一化结果。
 * 这类事件不是控制请求，不经过 command 路由；
 * runtime 直接把结果同步到 wm_window_t 中的 hint/capability 字段。
 */
typedef struct wm_window_hints_changed_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;
} wm_window_hints_changed_event_t;

/*
 * 归一化后的窗口激活请求来源。
 * 取值对齐 EWMH _NET_ACTIVE_WINDOW 的 source indication：
 * - 0: 旧客户端 / 未指明来源
 * - 1: 应用自身发起
 * - 2: pager / taskbar / window switcher 发起
 */
typedef enum wm_window_activation_source_t {
  WM_WINDOW_ACTIVATION_SOURCE_LEGACY      = 0,
  WM_WINDOW_ACTIVATION_SOURCE_APPLICATION = 1,
  WM_WINDOW_ACTIVATION_SOURCE_PAGER       = 2,
} wm_window_activation_source_t;

typedef struct wm_window_activate_request_event_t {
  wm_window_id_t window;
  wm_window_activation_source_t source;
} wm_window_activate_request_event_t;

/*
 * 归一化后的窗口模式切换请求。
 * backend 保留 add/remove/toggle 这类原始协议语义；
 * route_event() 再结合当前 wm_state_t 翻译成最终的 SET_* 命令。
 */
typedef enum wm_window_state_request_kind_t {
  WM_WINDOW_STATE_REQUEST_FULLSCREEN,
  WM_WINDOW_STATE_REQUEST_MAXIMIZED,
  WM_WINDOW_STATE_REQUEST_MINIMIZED,
} wm_window_state_request_kind_t;

typedef enum wm_window_state_request_action_t {
  WM_WINDOW_STATE_REQUEST_ACTION_ADD,
  WM_WINDOW_STATE_REQUEST_ACTION_REMOVE,
  WM_WINDOW_STATE_REQUEST_ACTION_TOGGLE,
} wm_window_state_request_action_t;

typedef struct wm_window_state_request_event_t {
  wm_window_id_t window;
  wm_window_state_request_kind_t kind;
  wm_window_state_request_action_t action;
} wm_window_state_request_event_t;

typedef enum wm_configure_request_field_t {
  WM_CONFIGURE_REQUEST_FIELD_NONE   = 0,
  WM_CONFIGURE_REQUEST_FIELD_X      = 1u << 0,
  WM_CONFIGURE_REQUEST_FIELD_Y      = 1u << 1,
  WM_CONFIGURE_REQUEST_FIELD_WIDTH  = 1u << 2,
  WM_CONFIGURE_REQUEST_FIELD_HEIGHT = 1u << 3,
} wm_configure_request_field_t;

/*
 * 归一化后的窗口配置请求。
 * 这类事件只表达稀疏几何请求，不承载 fullscreen/maximize/minimize
 * 之类窗口模式切换；后者应进入独立的 state request event。
 *
 * x / y / width / height 中只有 changed_fields 指示的成员才有意义。
 */
typedef struct wm_configure_request_event_t {
  wm_window_id_t window;
  uint32_t changed_fields;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} wm_configure_request_event_t;

typedef struct wm_event_t {
  wm_event_type_t type;
  // backend 归一化后的事件时间值（毫秒），不是 wall-clock 时间戳
  wm_time_ms_t time_ms;
  union {
    wm_key_press_event_t key_press;
    wm_pointer_button_event_t pointer_button_press;
    wm_pointer_button_event_t pointer_button_release;
    wm_pointer_motion_event_t pointer_motion;
    wm_pointer_enter_event_t pointer_enter;
    wm_window_map_request_event_t window_map_request;
    wm_window_remove_event_t window_remove;
    wm_window_metadata_changed_event_t window_metadata_changed;
    wm_window_hints_changed_event_t window_hints_changed;
    wm_window_activate_request_event_t window_activate_request;
    wm_window_state_request_event_t window_state_request;
    wm_configure_request_event_t configure_request;
  } data;
} wm_event_t;
