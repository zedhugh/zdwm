#include "action.h"

#include "monitor.h"
#include "wm.h"

void select_tag_of_current_monitor(const user_action_arg_t *arg) {
  monitor_select_tag(wm.current_monitor, arg->ui);
}
