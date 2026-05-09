#ifndef ZDWM_ACTION_H
#define ZDWM_ACTION_H

#include <stdint.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef union zdwm_action_arg_t {
  bool b;
  int32_t i;
  uint32_t ui;
  const char *str;
  const void *ptr;
} zdwm_action_arg_t;

typedef struct zdwm_runtime_t zdwm_runtime_t;

typedef struct zdwm_action_api_t {
  void (*spawn)(const char *command);
  void (*quit)(zdwm_runtime_t *runtime, bool restart);
  void (*raise_or_run)(
    zdwm_runtime_t *runtime,
    const char *class_name,
    const char *command
  );
  void (*toggle_fullscreen)(zdwm_runtime_t *runtime);
  void (*toggle_maximize)(zdwm_runtime_t *runtime);
  void (*toggle_floating)(zdwm_runtime_t *runtime);
  void (*toggle_sticky)(zdwm_runtime_t *runtime);
  void (*switch_workspace)(
    zdwm_runtime_t *runtime,
    zdwm_workspace_id_t workspace_id
  );
  void (*switch_workspace_in_current_output)(
    zdwm_runtime_t *runtime,
    zdwm_workspace_id_t workspace_id
  );
  void (*switch_workspace_by_index_in_current_output)(
    zdwm_runtime_t *runtime,
    uint32_t index
  );
  void (*cycle_layout)(zdwm_runtime_t *runtime, int32_t delta);
  void (*cycle_current_output)(zdwm_runtime_t *runtime, int32_t delta);
  void (*focus_window)(zdwm_runtime_t *runtime, int32_t delta);
} zdwm_action_api_t;

typedef void zdwm_action_fn(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *ctx,
  const zdwm_action_arg_t *arg
);

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_ACTION_H */
