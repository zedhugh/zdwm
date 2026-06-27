#pragma once

#include <cairo.h>
#include <stddef.h>

#include "interface/effect.h"
#include "interface/event.h"
#include "interface/tray.h"
#include "interface/types.h"

typedef struct backend_t backend_t;

typedef struct backend_detect_t {
  output_info_t *outputs;
  size_t output_count;
} backend_detect_t;

backend_t *backend_create(const char *display_name);
void backend_destroy(backend_t *backend);

backend_detect_t *backend_detect(backend_t *backend);
void backend_detect_destroy(backend_detect_t *detect);

int backend_get_fd(backend_t *backend);

/**
 * @brief 非阻塞地从 backend 中读取下一个归一化的事件
 *
 * @details
 * 1. 函数立即返回
 * 2. 只要取到了事件，函数就返回 true ，事件填充到 event 参数中
 *
 * @returns 如果还有下一个事件待读取，返回 true ，否则返回 false
 */
bool backend_poll_event(backend_t *backend, event_t *event);

/**
 * @brief 阻塞等待 backend 产出下一个可交给 runtime 的归一化事件
 * @details
 * backend 可以在内部丢弃无关的原始平台事件，直到拿到一个可路由事件或遇到
 * stop/error 条件。
 *
 * 返回 true 时，`event` 已被完整填充；若其中包含堆内存，所有权转交给调用方，
 * 调用方必须在本轮处理结束后调用 event_cleanup() 或 event_reset()。
 *
 * 返回 false 时，表示 backend 请求停止或遇到错误；此时 `event` 必须保持为
 * 无需 cleanup 的状态，调用方可以直接退出循环。
 */
bool backend_next_event(backend_t *backend, event_t *event);
bool backend_apply_effect(
  backend_t *backend,
  const effect_t *effects,
  size_t effect_count
);

typedef struct backend_scan_result_t {
  window_map_request_event_t *windows;
  size_t count;
  size_t capacity;
} backend_scan_result_t;

/**
 * @brief 扫描已有窗口
 *
 * @details 仅扫描窗口并获取其基础信息
 */
backend_scan_result_t *backend_scan_windows(backend_t *backend);
void backend_scan_result_destroy(backend_scan_result_t *result);

typedef struct backend_bar_window_t {
  window_id_t window_id;
  cairo_t *cr;
  tray_t tray;
} backend_bar_window_t;

backend_bar_window_t backend_create_bar_window(
  backend_t *backend,
  rect_t geometry,
  uint32_t bg_pixel,
  bool enable_tray
);
void backend_flush(backend_t *backend);
