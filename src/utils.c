#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"

const char *current_time_str(void) {
  static char buffer[25];
  time_t now;
  struct tm tm;

  time(&now);
  localtime_r(&now, &tm);
  if (!strftime(buffer, sizeof(buffer), "%Y-%m-%d %T", &tm)) buffer[0] = '\0';

  return buffer;
}

/** Print error and exit with EXIT_FAILURE code.
 */
void _fatal(int line, const char *fct, const char *file, const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  const char *format = "(EE) [%s] %s\n%s:%d:%s:\n";
  fprintf(stderr, format, APP_NAME, current_time_str(), file, line, fct);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}
