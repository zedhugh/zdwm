#pragma once

#include "core/command_buffer.h"
#include "core/event.h"
#include "core/plan.h"
#include "core/rules.h"
#include "core/state.h"

typedef struct policy_context_t {
  const state_t *state;
  const rules_t *rules;
} policy_context_t;

/**
 * @brief 事件路由：将运行时事件翻译为语义命令
 *
 * @param ctx   策略上下文
 * @param event 需要翻译的事件
 * @param out   命令输出缓冲区，路由产生的命令追加到此处
 *
 * @return true     产生了至少一条需要执行的命令
 * @return false    产生了至少一条需要执行的命令
 */
bool policy_route_event(
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
 *
 * @return true     产生了至少一项后端副作用
 * @return false    无副作用产生
 */
bool policy_apply_command(
  const policy_context_t *ctx,
  const command_buffer_t *command_buffer,
  plan_t *plan
);
