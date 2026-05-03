#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/types.h"

/* Internal semantic window types mapped from backend-specific protocols. */
typedef enum window_type_t {
  ZDWM_WINDOW_TYPE_NORMAL = 0,
  ZDWM_WINDOW_TYPE_DESKTOP,
  ZDWM_WINDOW_TYPE_DOCK,
  ZDWM_WINDOW_TYPE_TOOLBAR,
  ZDWM_WINDOW_TYPE_DIALOG,
  ZDWM_WINDOW_TYPE_UTILITY,
  ZDWM_WINDOW_TYPE_SPLASH,
  ZDWM_WINDOW_TYPE_MENU,
  ZDWM_WINDOW_TYPE_DROPDOWN_MENU,
  ZDWM_WINDOW_TYPE_POPUP_MENU,
  ZDWM_WINDOW_TYPE_TOOLTIP,
  ZDWM_WINDOW_TYPE_COMBO,
  ZDWM_WINDOW_TYPE_DND,
  ZDWM_WINDOW_TYPE_NOTIFICATION,
} window_type_t;

/* Internal semantic window states mapped from backend-specific protocols. */
typedef enum window_state_t {
  ZDWM_WINDOW_STATE_ABOVE = 0,
  ZDWM_WINDOW_STATE_FULLSCREEN,
  ZDWM_WINDOW_STATE_MODAL,
  ZDWM_WINDOW_STATE_STICKY,
} window_state_t;

/* 层级从低到高 */
typedef enum window_layer_type_t {
  ZDWM_WINDOW_LAYER_DESKTOP = 0,
  ZDWM_WINDOW_LAYER_NORMAL,
  ZDWM_WINDOW_LAYER_TOP,
  ZDWM_WINDOW_LAYER_OVERLAY,
  ZDWM_WINDOW_LAYER_COUNT,
} window_layer_type_t;

typedef enum window_metadata_change_flags_t {
  ZDWM_WINDOW_METADATA_CHANGE_TITLE    = 1u << 0,
  ZDWM_WINDOW_METADATA_CHANGE_APP_ID   = 1u << 1,
  ZDWM_WINDOW_METADATA_CHANGE_ROLE     = 1u << 2,
  ZDWM_WINDOW_METADATA_CHANGE_CLASS    = 1u << 3,
  ZDWM_WINDOW_METADATA_CHANGE_INSTANCE = 1u << 4,
} window_metadata_change_flags_t;

typedef struct window_layer_props_t {
  window_type_t *types;
  size_t type_count;

  window_state_t *states;
  size_t state_count;
} window_layer_props_t;

typedef struct window_metadata_t {
  char *title;
  char *app_id;
  char *role;
  char *class_name;
  char *instance_name;
} window_metadata_t;

typedef struct window_t {
  window_id_t id;
  window_id_t transient_for;
  workspace_id_t workspace_id;

  window_layer_type_t layer;
  bool fullscreen;
  bool maximized;
  bool minimized;
  bool floating;
  bool sticky;
  bool urgent;
  bool fixed_size;
  bool skip_taskbar;

  /* 几何信息（均为包含边框后的外框矩形） */
  rect_t float_rect; /* floating 模式下记忆的外框矩形 */
  rect_t frame_rect; /* 当前外框矩形（由 layout 或 float_rect 解析） */

  /* 元数据（核心算法不依赖，仅用于规则匹配和信息展示） */
  char *title;
  char *app_id;
  char *role;
  char *class_name;
  char *instance_name;

  uint32_t border_width;
} window_t;

window_layer_type_t window_classify_layer(const window_layer_props_t *props);

void window_layer_props_cleanup(window_layer_props_t *props);
void window_metadata_cleanup(window_metadata_t *metadata);

void window_set_fullscreen(window_t *window, bool fullscreen);
void window_set_maximized(window_t *window, bool maximized);
void window_set_minimized(window_t *window, bool minimized);
void window_set_floating(window_t *window, bool floating);
void window_set_sticky(window_t *window, bool sticky);
void window_set_urgent(window_t *window, bool urgent);
void window_set_fixed_size(window_t *window, bool fixed_size);
void window_set_skip_taskbar(window_t *window, bool skip_taskbar);
void window_set_float_rect(window_t *window, rect_t rect);
void window_set_frame_rect(window_t *window, rect_t rect);
void window_set_title(window_t *window, const char *title);
void window_set_app_id(window_t *window, const char *app_id);
void window_set_role(window_t *window, const char *role);
void window_set_class(window_t *window, const char *class_name);
void window_set_instance(window_t *window, const char *instance_name);
void window_set_border_width(window_t *window, uint32_t border_width);
void window_take_metadata(
  window_t *window,
  window_metadata_t *metadata,
  uint32_t changed_fields
);

/**
 * @brief 窗口是否需要参与布局计算
 */
bool window_need_layout(const window_t *window);
bool window_need_move(const window_t *window, int32_t x, int32_t y);
bool window_need_resize(const window_t *window, int32_t width, int32_t height);
bool window_should_has_border(const window_t *window);
