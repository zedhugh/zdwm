#pragma once

#include <stddef.h>

#include "interface/backend.h"

backend_detect_t *
output_remove_duplication(const output_info_t *output, const size_t count);
