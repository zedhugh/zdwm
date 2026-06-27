#pragma once

#include <stddef.h>
#include <stdint.h>

#include "interface/types.h"

typedef void (*tray_icons_change_cb_t)(void *user_data);

typedef struct tray_api_t {
  /**
   * @brief 获取 tray 当前挂载的 bar window id
   *
   * @detail    如果返回的 window_id 是 ZDWM_WINDOW_ID_INVALID 则表示 tray
   *            功能未开启，无 tray 需要挂载
   */
  window_id_t (*host_window)(void *handle);
  /* 获取当前 icon 数量 */
  size_t (*icon_count)(void *handle);
  /* 获取每个 icon 的边长， icon 长宽一样，都等于 bar_height */
  int32_t (*icon_size)(void *handle);
  /* 将 tray 的 icon 容器窗口起点放到 bar 中 x 这个位置 */
  void (*place)(void *handle, int32_t x);
  /* 设置 icon 数量变化回调 */
  void (*set_listener)(
    void *handle,
    tray_icons_change_cb_t cb,
    void *user_data
  );
} tray_api_t;

typedef struct tray_t {
  tray_api_t api;
  void *handle;
} tray_t;
