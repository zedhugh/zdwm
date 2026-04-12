#pragma once

#include <stddef.h>

#include "core/event.h"
#include "core/plan.h"
#include "core/types.h"

typedef struct backend_t backend_t;

typedef struct backend_detect_t {
  output_info_t *outputs;
  size_t output_count;
} backend_detect_t;

backend_t *backend_create(const char *display_name);
void backend_destroy(backend_t *backend);

backend_detect_t *backend_detect(backend_t *backend);
void backend_detect_destroy(backend_detect_t *detect);

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
bool backend_apply_effect(backend_t *backend, const effect_t *effects,
                          size_t effect_count);
