#include "action.h"

#include <glib.h>
#include <string.h>

#include "client.h"
#include "monitor.h"
#include "types.h"
#include "wm.h"

void focus_client_in_same_tag(const user_action_arg_t *arg) {
  bool next = arg->b;
  tag_t *tag = wm.current_monitor->selected_tag;

  task_in_tag_t *task = nullptr;
  if (next) {
    task = client_get_next_task_in_tag(wm.client_focused, tag);
  } else {
    task = client_get_previous_task_in_tag(wm.client_focused, tag);
  }

  if (!task) return;

  client_stack_raise(task->client);
  client_focus(task->client);
}

void select_tag_of_current_monitor(const user_action_arg_t *arg) {
  monitor_select_tag(wm.current_monitor, arg->ui);
}

void send_client_to_tag(const user_action_arg_t *arg) {
  if (wm.client_focused) client_send_to_tag(wm.client_focused, arg->ui);
}

void send_client_to_next_monitor(const user_action_arg_t *arg) {
  if (!wm.client_focused) return;

  monitor_t *current_monitor = wm.client_focused->monitor;
  monitor_t *next_monitor = current_monitor->next;
  if (next_monitor == nullptr) {
    next_monitor = wm.monitor_list;
  } else {
    for (monitor_t *m = wm.monitor_list; m && m->next != current_monitor;
         m = m->next) {
      next_monitor = m;
    }
  }

  if (current_monitor != next_monitor) {
    client_send_to_monitor(wm.client_focused, next_monitor);
  }
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
