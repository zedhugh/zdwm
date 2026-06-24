#pragma once

#include "interface/types.h"

/*
 * 由 backend 提供的窗口基础信息。
 *
 * 这些字符串由调用方提供；state 如需长期持有，应自行复制。
 */
typedef struct window_info_t {
  window_id_t id;
  window_id_t transient_for;
  rect_t frame_rect;

  const char *title;
  const char *app_id;
  const char *role;
  const char *class_name;
  const char *instance_name;

  window_layer_type_t layer_type;
  bool fullscreen;
  bool maximized;
  bool minimized;
  bool urgent;
  bool skip_taskbar;

  zdwm_size_t min_size;
  zdwm_size_t max_size;
} window_info_t;
