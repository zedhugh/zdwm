#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ID 类型定义
// workspace/output/layout 在启动时分配固定数量，运行时不再增删
// window 由后端管理，使用后端原生 ID
typedef uint32_t wm_window_id_t;
typedef uint16_t wm_workspace_id_t;
typedef uint16_t wm_output_id_t;
typedef uint32_t wm_layout_id_t;
/*
 * backend 归一化后的毫秒时间值。
 * 它对齐 X11 / Wayland 常见的 32-bit ms 时间域，用于事件排序和短时差计算；
 * 不是 wall-clock / Unix epoch，基准也不保证可见，因此允许回绕。
 */
typedef uint32_t wm_time_ms_t;
/*
 * 逻辑键编码，使用 xkb keysym 值空间。
 * 它表示“当前 keymap 下，这次按键对应的逻辑主键”，而不是平台原始扫描码。
 */
typedef uint32_t wm_keysym_t;
typedef uint32_t wm_modifier_mask_t;
/*
 * 通用 32-bit RGBA 颜色值，编码为 0xRRGGBBAA。
 * backend 若不支持 alpha，可忽略最低 8 bit 或在提交平台 effect 时自行量化。
 */
typedef uint32_t wm_rgba32_t;

// 无效 ID 标记
#define WM_WINDOW_ID_INVALID    ((wm_window_id_t)0)
#define WM_WORKSPACE_ID_INVALID ((wm_workspace_id_t) - 1)
#define WM_OUTPUT_ID_INVALID    ((wm_output_id_t) - 1)
#define WM_LAYOUT_ID_INVALID    ((wm_layout_id_t) - 1)
#define WM_KEYSYM_INVALID       ((wm_keysym_t)0)

/*
 * 归一化后的修饰键掩码：
 * - backend 应在产出 wm_event_t 之前合并 left/right 变体
 * - CapsLock / NumLock 这类锁定位不应进入该掩码
 */
typedef enum wm_modifier_bit_t {
  WM_MOD_NONE  = 0,
  WM_MOD_SHIFT = 1u << 0,
  WM_MOD_CTRL  = 1u << 1,
  WM_MOD_ALT   = 1u << 2,
  WM_MOD_SUPER = 1u << 3,
} wm_modifier_bit_t;

/*
 * 归一化后的指针按键语义。
 * backend 应把平台原始按钮码映射到这些稳定枚举值；
 * 若需要保留平台原始值，可放在事件里的 raw_button 字段。
 */
typedef enum wm_button_t {
  WM_BUTTON_NONE = 0,
  WM_BUTTON_LEFT,
  WM_BUTTON_MIDDLE,
  WM_BUTTON_RIGHT,
  WM_BUTTON_WHEEL_UP,
  WM_BUTTON_WHEEL_DOWN,
  WM_BUTTON_WHEEL_LEFT,
  WM_BUTTON_WHEEL_RIGHT,
  WM_BUTTON_BACK,
  WM_BUTTON_FORWARD,
} wm_button_t;

typedef struct wm_point_t {
  int32_t x;
  int32_t y;
} wm_point_t;

typedef struct wm_rect_t {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
} wm_rect_t;

/*
 * 核心统一使用 geometry_mode 表示窗口的显示几何模式。
 * 其中 WM_GEOMETRY_MINIMIZED 覆盖所有“被最小化/被图标化”的不可见状态：
 * - 在 X11 上，它对应 ICCCM 的 IconicState
 * - 来自 WM_CHANGE_STATE(Iconic) 的请求也应折叠到这个模式
 *
 * 最小核心不单独暴露 "iconic" 枚举，避免与 minimized 形成两套并行语义。
 */
typedef enum wm_window_geometry_mode_t {
  WM_GEOMETRY_NORMAL,
  WM_GEOMETRY_MAXIMIZED,
  WM_GEOMETRY_FULLSCREEN,
  WM_GEOMETRY_MINIMIZED,
} wm_window_geometry_mode_t;

typedef enum wm_focus_direction_t {
  WM_FOCUS_PREV,
  WM_FOCUS_NEXT,
} wm_focus_direction_t;

typedef enum wm_cross_output_policy_t {
  WM_CROSS_OUTPUT_KEEP_WORKSPACE,
  WM_CROSS_OUTPUT_MOVE_TO_TARGET_WORKSPACE,
} wm_cross_output_policy_t;
