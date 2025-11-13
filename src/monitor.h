#pragma once

#include <stdint.h>

#include "types.h"

void monitor_initialize_tag(monitor_t *monitor, const char **tags);
void monitor_select_tag(monitor_t *monitor, uint32_t tag_mask);
