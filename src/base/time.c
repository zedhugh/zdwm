#include "base/time.h"

#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>

#include "base/log.h"

static int time_create_monotonic_timerfd(const struct itimerspec *itimerspec) {
  auto timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
  if (timerfd == -1) {
    auto err = errno;
    warn("time_create_timerfd failed: %s", strerror(err));
    return timerfd;
  }

  timerfd_settime(timerfd, 0, itimerspec, nullptr);
  return timerfd;
}

static constexpr auto NANOSECONDS_OF_ONE_SECOND = 1'000'000'000ULL;

int time_create_monotonic_timerfd_by_fps(uint32_t fps) {
  if (fps == 0) fps = 30;

  auto interval_ns = NANOSECONDS_OF_ONE_SECOND / fps;

  /* clang-format off */
  struct itimerspec timer_spec = {
    .it_value = {
      .tv_sec  = 0,
      .tv_nsec = interval_ns % NANOSECONDS_OF_ONE_SECOND,
    },
    .it_interval = {
      .tv_sec  = interval_ns / NANOSECONDS_OF_ONE_SECOND,
      .tv_nsec = interval_ns % NANOSECONDS_OF_ONE_SECOND,
    },
  };
  /* clang-format on */

  return time_create_monotonic_timerfd(&timer_spec);
}

static uint64_t time_monotonic_ns(void) {
  struct timespec timestamp = {0};
  if (clock_gettime(CLOCK_MONOTONIC, &timestamp) == -1) {
    warn("clock_gettime failed: %s", strerror(errno));
  }
  return timestamp.tv_sec * NANOSECONDS_OF_ONE_SECOND + timestamp.tv_nsec;
}

uint64_t time_monotonic_ms(void) { return time_monotonic_ns() / 1'000'000U; }
