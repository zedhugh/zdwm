#pragma once

#include <stdint.h>

int time_create_monotonic_timerfd_by_fps(uint32_t fps);
uint64_t time_monotonic_ms(void);
