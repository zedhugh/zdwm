#pragma once

#include "wm_command.h"
#include "wm_types.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * 输入绑定只匹配 backend 已归一化后的输入事件。
 * backend 负责把平台原始输入翻译为 wm_event_t，并决定是否折叠 auto-repeat。
 */
typedef enum wm_binding_match_mode_t {
  // modifiers 必须与事件完全一致
  WM_BINDING_MATCH_EXACT,
  // 事件允许带有更多修饰键，但必须至少包含 binding.modifiers
  WM_BINDING_MATCH_ALLOW_EXTRA_MODIFIERS,
} wm_binding_match_mode_t;

typedef struct wm_key_binding_t {
  wm_keysym_t keysym;
  wm_modifier_mask_t modifiers;
  wm_binding_match_mode_t match_mode;
  // 命中的绑定会直接复制这条命令模板进入 command buffer
  wm_command_t command;
} wm_key_binding_t;

typedef struct wm_key_binding_table_t {
  const wm_key_binding_t *items;
  size_t count;
} wm_key_binding_table_t;

/*
 * 核心只关心 root / window 两类命中目标：
 * - bar / status / widget 等点击区域属于核心外服务，不进入最小核心绑定系统
 * - 当事件未命中受管窗口时，wm_event_t.window == WM_WINDOW_ID_INVALID
 */
typedef enum wm_pointer_binding_target_t {
  WM_POINTER_BINDING_TARGET_ANY,
  WM_POINTER_BINDING_TARGET_ROOT,
  WM_POINTER_BINDING_TARGET_WINDOW,
} wm_pointer_binding_target_t;

/*
 * 最小核心当前只定义“按下触发”的鼠标绑定；
 * button release 主要保留给 move/resize 交互结束逻辑。
 *
 * 若 command 是窗口目标命令，且模板中的 window_id 为
 * WM_WINDOW_ID_INVALID，则 route_pointer_press() 应把它替换为当前点击的
 * event->window。
 */
typedef struct wm_pointer_binding_t {
  wm_button_t button;
  wm_modifier_mask_t modifiers;
  wm_binding_match_mode_t match_mode;
  wm_pointer_binding_target_t target;
  wm_command_t command;
} wm_pointer_binding_t;

typedef struct wm_pointer_binding_table_t {
  const wm_pointer_binding_t *items;
  size_t count;
} wm_pointer_binding_table_t;

/*
 * 查找与当前按键事件匹配的绑定。
 * 匹配顺序固定为：
 * 1. 所有 exact 绑定，按数组顺序扫描
 * 2. 所有 allow-extra-modifiers 绑定，按数组顺序扫描
 *
 * 当前最小核心不内建“默认绑定生成规则”；像 SUPER+1 这类行为也必须显式
 * 出现在 key binding table 里。
 */
const wm_key_binding_t *wm_key_binding_find(
    const wm_key_binding_table_t *table, wm_keysym_t keysym,
    wm_modifier_mask_t modifiers);

/*
 * 查找与当前按钮按下事件匹配的鼠标绑定。
 * hits_window == true 表示事件命中了某个受管窗口；
 * hits_window == false 表示事件落在 root / 空白区域。
 *
 * 匹配顺序与键盘绑定一致：先 exact，再 allow-extra-modifiers。
 */
const wm_pointer_binding_t *wm_pointer_binding_find(
    const wm_pointer_binding_table_t *table, wm_button_t button,
    wm_modifier_mask_t modifiers, bool hits_window);
