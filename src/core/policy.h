#pragma once

#include <zdwm/action.h>

#include "core/binding.h"
#include "core/command_buffer.h"
#include "core/event.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"

typedef struct policy_context_t {
  binding_table_t *bind_table;
  state_t *state;
  const rules_t *rules;
  const zdwm_action_ctx_t action_ctx;
} policy_context_t;

/**
 * @brief 事件路由：将运行时事件翻译为语义命令
 *
 * @param ctx   策略上下文
 * @param event 需要翻译的事件
 * @param out   命令输出缓冲区，路由产生的命令追加到此处
 */
void policy_route_event(
  const policy_context_t *ctx,
  const event_t *event,
  command_buffer_t *out
);

/**
 * @brief 将语义命令应用到状态，并累积后端副作用到 plan
 *
 * @param ctx               策略上下文
 * @param command_buffer    待应用的命令序列
 * @param plan              副作用累积器，记录后端需要执行的操作
 */
void policy_apply_command(
  const policy_context_t *ctx,
  const command_buffer_t *command_buffer,
  plan_t *plan
);
