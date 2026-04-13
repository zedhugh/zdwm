#pragma once

#include <stdarg.h>
#include <stdio.h>

#define fatal(format, ...) \
  _fatal(__LINE__, __FUNCTION__, __FILE__, format, ##__VA_ARGS__)
void _fatal(
  int line,
  const char *function,
  const char *file,
  const char *format,
  ...
) __attribute__((noreturn)) __attribute__((format(printf, 4, 5)));

#define warn(format, ...) \
  _warn(__LINE__, __FUNCTION__, __FILE__, format, ##__VA_ARGS__)
void _warn(
  int line,
  const char *function,
  const char *file,
  const char *format,
  ...
) __attribute__((format(printf, 4, 5)));

const char *current_time_str(void);

#if defined(RELEASE)
#define logger(format, ...)
#elif defined(LOG_FILE)
static inline void logger(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  FILE *log_file = fopen(LOG_FILE, "a+t");
  vfprintf(log_file, format, ap);
  va_end(ap);
  fclose(log_file);
}
#else
#define logger(format, ...) printf(format, ##__VA_ARGS__)
#endif
