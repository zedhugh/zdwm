#pragma once

#include "wm_types.h"

/*
 * wm_event_t 表示“runtime 真正关心的输入事实”：
 * - 它不是平台原始事件的逐字段照抄
 * - 它允许 backend 对平台事件做最小必要的归一化 / 过滤 / 聚合
 * - 它不承载 runtime 控制语义；stop / restart / error 由 backend.next_event()
 *   的返回结果表达，而不是伪装成一个事件类型
 */
typedef enum wm_event_type_t {
  WM_EVENT_KEY_PRESS,
  WM_EVENT_POINTER_BUTTON_PRESS,
  WM_EVENT_POINTER_BUTTON_RELEASE,
  WM_EVENT_POINTER_MOTION,
  WM_EVENT_WINDOW_MAP_REQUEST,
  WM_EVENT_WINDOW_UNMAP,
  WM_EVENT_WINDOW_DESTROY,
  WM_EVENT_WINDOW_METADATA_CHANGED,
  WM_EVENT_CONFIGURE_REQUEST,
  WM_EVENT_TIMER_TICK,
  WM_EVENT_STATUS_TICK,
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

typedef struct wm_window_map_request_event_t {
  wm_window_id_t window;
} wm_window_map_request_event_t;

typedef struct wm_window_unmap_event_t {
  wm_window_id_t window;
} wm_window_unmap_event_t;

typedef struct wm_window_destroy_event_t {
  wm_window_id_t window;
} wm_window_destroy_event_t;

typedef enum wm_window_meta_changed_flags_t {
  WM_WINDOW_META_CHANGED_NONE = 0,
  WM_WINDOW_META_CHANGED_TITLE = 1u << 0,
  WM_WINDOW_META_CHANGED_APP_ID = 1u << 1,
  WM_WINDOW_META_CHANGED_CLASS = 1u << 2,
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

typedef struct wm_configure_request_event_t {
  wm_window_id_t window;
  wm_rect_t requested_rect;
} wm_configure_request_event_t;

typedef struct wm_timer_tick_event_t {
  wm_time_ms_t now_ms;
} wm_timer_tick_event_t;

typedef struct wm_status_tick_event_t {
  wm_time_ms_t now_ms;
} wm_status_tick_event_t;

typedef struct wm_event_t {
  wm_event_type_t type;
  // backend 归一化后的事件时间值（毫秒），不是 wall-clock 时间戳
  wm_time_ms_t time_ms;
  union {
    wm_key_press_event_t key_press;
    wm_pointer_button_event_t pointer_button_press;
    wm_pointer_button_event_t pointer_button_release;
    wm_pointer_motion_event_t pointer_motion;
    wm_window_map_request_event_t window_map_request;
    wm_window_unmap_event_t window_unmap;
    wm_window_destroy_event_t window_destroy;
    wm_window_metadata_changed_event_t window_metadata_changed;
    wm_configure_request_event_t configure_request;
    wm_timer_tick_event_t timer_tick;
    wm_status_tick_event_t status_tick;
  } data;
} wm_event_t;
