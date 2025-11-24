#include "action.h"

#include <glib.h>
#include <string.h>

#include "monitor.h"
#include "wm.h"

void select_tag_of_current_monitor(const user_action_arg_t *arg) {
  monitor_select_tag(wm.current_monitor, arg->ui);
}

void spawn(const user_action_arg_t *arg) {
  const char *cmd = (const char *)arg->ptr;
  if (cmd == nullptr || strlen(cmd) == 0) return;

  g_spawn_command_line_async((const char *)arg->ptr, nullptr);
}

void quit(const user_action_arg_t *arg) {
  const bool restart = arg->b;
  if (restart) {
    wm_restart();
  } else {
    wm_quit();
  }
}
