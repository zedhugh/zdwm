#include "layout.h"

#include <stdint.h>

#include "base.h"
#include "client.h"
#include "types.h"
#include "utils.h"

void monocle(tag_t *tag) {
  logger("== monocle tag: %s start ==\n", tag->name);
  for (task_in_tag_t *task = tag->task_list; task; task = task->next) {
    if (!client_need_layout(task->client)) continue;

    uint16_t border_width = task->client->border_width;
    area_t workarea = task->client->monitor->workarea;
    task->geometry.x = workarea.x;
    task->geometry.y = workarea.y;
    task->geometry.width = workarea.width - border_width * 2;
    task->geometry.height = workarea.height - border_width * 2;
    logger(
      "== window 0x%x: x: %d, y: %d, width: %u, height: %u, border width: %u\n",
      task->client->window, task->geometry.x, task->geometry.y,
      task->geometry.width, task->geometry.height, border_width);
  }
  logger("== monocle tag: %s end ==\n", tag->name);
}
