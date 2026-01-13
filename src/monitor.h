#pragma once

#include <stdint.h>

#include "types.h"

uint32_t monitor_initialize_tag(monitor_t *monitor, const char **tags,
                                uint32_t tag_index_start_at);
void monitor_deal_focus(monitor_t *monitor);
void monitor_select_tag(monitor_t *monitor, uint32_t tag_mask);
void monitor_clean(monitor_t *monitor);
void monitor_init_bar(monitor_t *monitor);
void monitor_draw_bar(monitor_t *monitor);
void monitor_arrange(monitor_t *monitor);
void monitor_save_cursor_point(monitor_t *monitor);
void monitor_restore_cursor_point(monitor_t *monitor);
