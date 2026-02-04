#pragma once

#include <glib.h>
#include <stdint.h>

typedef struct memory_usage_t {
  char mem_used_text[32];
  char swap_used_text[32];

  float mem_percent;
  float swap_percent;
  uint64_t mem_used;
  uint64_t swap_used;

  uint64_t mem_total;
  uint64_t mem_free;
  uint64_t buffers;
  uint64_t cached;
  uint64_t swap_total;
  uint64_t swap_free;
  uint64_t s_reclaimable;
} memory_usage_t;

typedef struct net_speed_t {
  uint64_t rx_bytes;
  uint64_t tx_bytes;
  char up[64];
  char down[64];
} net_speed_t;

typedef struct status_t {
  net_speed_t net_speed;
  memory_usage_t mem_usage;
  double cpu_usage_percent;
  char time[64];
} status_t;

typedef void (*status_changed_notify)(status_t *status);
void init_status(GMainContext *context, status_changed_notify callback);
void clean_status(void);
