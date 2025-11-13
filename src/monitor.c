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

static void tag_clean(tag_t *tag) {
  tag_t *next_tag = nullptr;
  for (tag_t *t = tag; t; t = next_tag) {
    p_delete(&t->name);

    next_tag = t->next;
    p_delete(&t);
  }
}

void monitor_clean(monitor_t *monitor) {
  monitor_t *next_monitor = nullptr;
  for (monitor_t *m = monitor; m; m = next_monitor) {
    p_delete(&m->name);
    tag_clean(m->tag_list);

    m->selected_tag = nullptr;
    m->tag_list = nullptr;

    next_monitor = m->next;
    p_delete(&m);
  }
}
