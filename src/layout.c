#include "layout.h"

#include <stdint.h>

#include "client.h"
#include "types.h"
#include "utils.h"

void monocle(tag_t *tag) {
  logger("== monocle tag: %s start ==\n", tag->name);
  for (task_in_tag_t *task = tag->task_list; task; task = task->next) {
    if (!client_need_layout(task->client)) continue;
    task->geometry = task->client->monitor->workarea;
    logger("== window 0x%x: x: %d, y: %d, width: %u, height: %u\n",
           task->client->window, task->geometry.x, task->geometry.y,
           task->geometry.width, task->geometry.height);
  }
  logger("== monocle tag: %s end ==\n", tag->name);
}
