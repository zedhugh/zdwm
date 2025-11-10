#include "backtrace.h"

#include "app.h"
#include "buffer.h"

#ifdef HAS_EXECINFO
#include <execinfo.h>
#endif

#define MAX_STACK_SIZE 32

/**
 * Get a backtrace.
 * @param buffer The buffer to fill with backtrace.
 */
void backtrace_get(buffer_t *buffer) {
  buffer_init(buffer);

#ifdef HAS_EXECINFO
  void *stack[MAX_STACK_SIZE];
  char **bt;
  int stack_size;

  stack_size = backtrace(stack, countof(stack));
  bt = backtrace_symbols(stack, stack_size);

  if (bt) {
    for (int i = 0; i < stack_size; i++) {
      if (i > 0) buffer_addsl(buffer, "\n");
      buffer_adds(buffer, bt[i]);
    }
    p_delete(&bt);
  } else
#endif

    buffer_addsl(buffer, "Cannot get backtrace symbols.");
}
