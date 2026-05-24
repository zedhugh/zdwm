#ifndef ZDWM_LISTENERS_H
#define ZDWM_LISTENERS_H

#include <stddef.h>
#include <zdwm/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void
zdwm_current_output_id_listener(zdwm_output_id_t output_id, void *user_data);

typedef struct zdwm_workspace_t {
  zdwm_output_id_t output_id;
  zdwm_workspace_id_t id;
  const char *name; /* 不持有内存 */
} zdwm_workspace_t;
typedef void zdwm_initial_workspace_list(
  const zdwm_workspace_t *list,
  size_t count,
  void *user_data
);
typedef void zdwm_workspace_active_updated(
  zdwm_output_id_t output_id,
  zdwm_workspace_id_t workspace_id,
  void *user_data
);

typedef struct zdwm_layout_notify_t {
  zdwm_workspace_id_t workspace;
  zdwm_layout_id_t id;
  const char *name;        /* 不持有内存 */
  const char *symbol;      /* 不持有内存 */
  const char *description; /* 不持有内存 */
} zdwm_layout_notify_t;
typedef void zdwm_layout_notify(zdwm_layout_notify_t layout, void *user_data);

typedef struct zdwm_binding_mode_notify_t {
  bool is_default_mode;
  zdwm_binding_mode_id_t id;
  const char *name; /* 不持有内存 */
} zdwm_binding_mode_notify_t;
typedef void zdwm_binding_mode_notify(
  zdwm_binding_mode_notify_t binding_mode,
  void *user_data
);

typedef struct zdwm_window_t {
  zdwm_workspace_id_t workspace;
  zdwm_window_id_t id;
  const char *title; /* 不持有内存 */
  bool focused;
  bool urgent;
  bool skip_taskbar;
} zdwm_window_t;
typedef void zdwm_initial_window_list(
  const zdwm_window_t *list,
  size_t count,
  void *user_data
);
typedef void zdwm_window_added(const zdwm_window_t *window, void *user_data);
typedef void zdwm_window_updated(const zdwm_window_t *window, void *user_data);
typedef void zdwm_window_removed(zdwm_window_id_t window_id, void *user_data);

#if defined(__cplusplus)
}
#endif

#endif /* ZDWM_LISTENERS_H */
