#pragma once

#include <stddef.h>
#include <sys/types.h>

#define ssizeof(foo) (ssize_t)sizeof(foo)
#define countof(foo) (ssizeof(foo) / ssizeof(foo[0]))

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect((expr), 0)
#else
#define likely(expr) expr
#define unlikely(expr) expr
#endif

#ifdef MIN
#undef MIN
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef MAX
#undef MAX
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
