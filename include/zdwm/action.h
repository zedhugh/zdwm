#ifndef ZDWM_ACTION_H
#define ZDWM_ACTION_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef union zdwm_action_arg_t {
  bool b;
  int32_t i;
  uint32_t ui;
  const char *str;
} zdwm_action_arg_t;

typedef struct zdwm_action_ctx_t zdwm_action_ctx_t;

typedef void
zdwm_action_fn(const zdwm_action_ctx_t *ctx, const zdwm_action_arg_t *arg);

struct zdwm_action_ctx_t {
  void (*spawn)(const char *command);
};

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_ACTION_H */
