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

  monitor_t *next_monitor = wm_get_next_monitor(wm.client_focused->monitor);
  if (wm.client_focused->monitor == next_monitor) return;

  client_send_to_monitor(wm.client_focused, next_monitor);
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

void raise_or_run(const user_action_arg_t *arg) {
  const char *class = ((const char **)arg->ptr)[0];
  client_t *client = client_get_next_by_class(wm.client_focused, class);
  if (client && client == wm.client_focused) return;

  if (client) {
    wm.current_monitor = client->monitor;
    monitor_select_tag(client->monitor, client->tags);
    client_stack_raise(client);
    client_focus(client);
    return;
  }

  const char *command = ((const char **)arg->ptr)[1];
  user_action_arg_t arguments = {.ptr = command};
  spawn(&arguments);
}
