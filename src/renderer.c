#include "renderer.h"

#include <stdio.h>
#include <string.h>

#include "color.h"
#include "config.h"
#include "status.h"
#include "utils.h"

static void status_text_from_data(status_type_t type, const status_t *status,
                                  char *text, size_t text_size) {
  if (!text || text_size == 0) return;
  text[0] = '\0';
  if (!status) return;

  switch (type) {
    case status_net_down:
      snprintf(text, text_size, "%s", status->net_speed.down);
      break;
    case status_net_up:
      snprintf(text, text_size, "%s", status->net_speed.up);
      break;
    case status_audio:
      if (status->pulse) {
        snprintf(text, text_size, "%d%%%s%s%s", status->pulse->volume_percent,
                 status->pulse->mute ? "M" : "",
                 strlen(status->pulse->device_name) ? "|" : "",
                 status->pulse->device_name);
      } else {
        snprintf(text, text_size, "0%%");
      }
      break;
    case status_memory:
      snprintf(text, text_size, "%s(%.1f%%)", status->mem_usage.mem_used_text,
               status->mem_usage.mem_percent);
      break;
    case status_cpu:
      snprintf(text, text_size, "%.1lf", status->cpu_usage_percent);
      break;
    case status_time:
      snprintf(text, text_size, "%s", status->time);
      break;
    default:
      break;
  }
}

void renderer_render_status(config_status_t *config, size_t config_count,
                            status_t *status, renderer_status_t *output) {
  static color_t *color_cache = nullptr;
  static size_t color_cache_count = 0;

  if (!config || !output) return;

  size_t item_count = config->status_count;
  if (config_count && config_count < item_count) item_count = config_count;

  output->gap = config->status_gap;
  output->item_gap = config->status_item_gap;
  output->item_count = item_count;

  if (item_count == 0) return;

  if (color_cache_count < item_count) {
    xrealloc((void **)&color_cache, (ssize_t)(sizeof(color_t) * item_count));
    color_cache_count = item_count;
  }

  for (size_t i = 0; i < item_count; i++) {
    const config_status_item_t *config_item = &config->list[i];
    renderer_status_item_t *output_item = &output->item_list[i];
    memset(output_item, 0, sizeof(*output_item));

    output_item->icon_type = config_item->icon_type;
    if (config_item->icon_type == icon_type_text) {
      output_item->icon_text = config_item->icon;
    } else if (config_item->icon_type == icon_type_image) {
      output_item->icon_path = config_item->icon;
    }

    status_text_from_data(config_item->type, status, output_item->text,
                          sizeof(output_item->text));

    if (config_item->color && *config_item->color) {
      color_parse(config_item->color, &color_cache[i]);
      output_item->color = &color_cache[i];
    }
  }
}
