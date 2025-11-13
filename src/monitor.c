#include "monitor.h"

#include <string.h>

#include "types.h"
#include "utils.h"

void monitor_initialize_tag(monitor_t *monitor, const char **tags) {
  int i = 0;
  const char *tag_name = nullptr;
  tag_t *prev_tag = nullptr;
  for (tag_name = tags[i]; tag_name; tag_name = tags[++i]) {
    tag_t *tag = p_new(tag_t, 1);
    tag->name = strdup(tag_name);
    tag->mask = 1u << i;

    if (prev_tag) {
      prev_tag->next = tag;
    } else {
      monitor->tag_list = tag;
    }

    prev_tag = tag;
  }

  monitor->selected_tag = monitor->tag_list;
}
