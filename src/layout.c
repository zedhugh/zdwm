#include "layout.h"

#include <math.h>
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

void tile(tag_t *tag) {
  logger("== tile tag: %s start ==\n", tag->name);
  uint16_t amount = 0;
  for (task_in_tag_t *task = tag->task_list; task; task = task->next) {
    if (client_need_layout(task->client)) ++amount;
  }
  if (amount == 0) goto end;

  uint16_t columns = (uint32_t)floor(sqrt(amount));
  if (columns * (columns + 1) <= amount) ++columns;

  uint32_t rows_in_other_cols = amount / columns;
  uint32_t rows_in_main_col;
  while ((rows_in_main_col = amount - (columns - 1) * rows_in_other_cols) >
         rows_in_other_cols) {
    ++rows_in_other_cols;
  }

  area_t *workarea = &tag->task_list->client->monitor->workarea;

  uint16_t width_avg = workarea->width / columns;
  uint16_t width_for_main_col = workarea->width - (columns - 1) * width_avg;
  uint16_t width_for_other_cols = width_avg;

  uint16_t i = 0, row = 0, col = 0, row_count, width, height;
  int16_t x = workarea->x, y = workarea->y;
  for (task_in_tag_t *task = tag->task_list; task; task = task->next) {
    if (!client_need_layout(task->client)) continue;

    if (i < rows_in_main_col) {
      row = i;
      width = width_for_main_col;
      row_count = rows_in_main_col;
    } else {
      width = width_for_other_cols;
      row_count = rows_in_other_cols;
      if (((i - rows_in_main_col) % row_count) == 0) {
        col++;
        row = 0;
        x += col == 1 ? width_for_main_col : width_for_other_cols;
        y = workarea->y;
      }
    }

    uint16_t h_avg = workarea->height / row_count;
    uint16_t h_main = workarea->height - h_avg * (row_count - 1);
    height = row == 0 ? h_main : h_avg;
    task->geometry.x = x;
    task->geometry.y = y;
    task->geometry.width = width - task->client->border_width * 2;
    task->geometry.height = height - task->client->border_width * 2;
    y += height;

    ++i;

    logger(
      "== window 0x%x: x: %d, y: %d, width: %u, height: %u, border width: %u\n",
      task->client->window, task->geometry.x, task->geometry.y,
      task->geometry.width, task->geometry.height, task->client->border_width);
  }

end:
  logger("== tile tag: %s end\n", tag->name);
}
