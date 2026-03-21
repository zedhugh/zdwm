#pragma once

#include <stddef.h>

#include "core/backend.h"
#include "core/wm_desc.h"

wm_backend_detect_t *output_remove_duplication(const wm_output_info_t *output,
                                               const size_t count);
