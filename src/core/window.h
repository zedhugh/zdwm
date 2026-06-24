#pragma once

#include <stddef.h>
#include <stdint.h>

#include "interface/types.h"

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
  bool skip_taskbar;

  zdwm_size_t min_size;
  zdwm_size_t max_size;

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

void window_set_fullscreen(window_t *window, bool fullscreen);
void window_set_maximized(window_t *window, bool maximized);
void window_set_minimized(window_t *window, bool minimized);
void window_set_floating(window_t *window, bool floating);
void window_set_sticky(window_t *window, bool sticky);
void window_set_urgent(window_t *window, bool urgent);
void window_set_skip_taskbar(window_t *window, bool skip_taskbar);
void window_set_float_rect(window_t *window, rect_t rect);
void window_set_frame_rect(window_t *window, rect_t rect);
void window_set_size_hint(window_t *window, zdwm_size_t min, zdwm_size_t max);
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
bool window_should_fix_size(const window_t *window);
bool window_can_resize(const window_t *window);
