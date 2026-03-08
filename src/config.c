#include "config.h"

#include <string.h>

#include "default_config.h"
#include "utils.h"

void config_status_set_gap(config_status_t *status_config, uint32_t gap,
                           uint32_t item_gap) {
  if (!status_config) fatal("no valid status config");
  status_config->status_gap = gap;
  status_config->status_item_gap = item_gap;
}

void config_status_add_item(config_status_t **status_config,
                            config_status_item_t status_item) {
  const auto item_count = (*status_config)->status_count + 1;
  const auto new_size =
    sizeof(config_status_t) + item_count * sizeof(config_status_item_t);
  xrealloc((void **)status_config, (ssize_t)new_size);
  (*status_config)->list[item_count - 1] = status_item;
  (*status_config)->status_count = item_count;
}

void config_status_filter_item(config_status_t **status_config,
                               status_item_filter filter) {
  if (!status_config || !*status_config) fatal("no valid status config");
  if (!filter) return;

  config_status_t *config = *status_config;
  size_t write_idx = 0;
  size_t item_count = config->status_count;

  for (size_t read_idx = 0; read_idx < item_count; read_idx++) {
    config_status_item_t *item = &config->list[read_idx];
    if (!filter(item)) continue;
    if (write_idx != read_idx) {
      config->list[write_idx] = *item;
    }
    write_idx++;
  }

  if (write_idx == item_count) return;

  config->status_count = write_idx;
  size_t new_size =
    sizeof(config_status_t) + write_idx * sizeof(config_status_item_t);
  xrealloc((void **)status_config, (ssize_t)new_size);
}

static config_status_t *config_create_default_status(void) {
  size_t item_count = countof(status_list);
  size_t size = sizeof(config_status_t) + item_count * sizeof(status_list[0]);
  config_status_t *status = xmalloc((ssize_t)size);

  status->status_gap = status_config.status_gap;
  status->status_item_gap = status_config.status_item_gap;
  status->status_count = item_count;
  memcpy(status->list, status_list, item_count * sizeof(status_list[0]));

  return status;
}

static config_t runtime_config = {};

const config_t *init_config(void) {
  if (!runtime_config.status) {
    runtime_config.status = config_create_default_status();
  }
  if (!runtime_config.status_renderer) {
    runtime_config.status_renderer = renderer_render_status;
  }
  return &runtime_config;
}

void config_set_custom_status_renderer(config_t *config,
                                       status_renderer_t status_renderer) {
  if (!config) fatal("no valid config");
  config->status_renderer =
    status_renderer ? status_renderer : renderer_render_status;
}
