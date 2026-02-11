#include "status.h"

#include <glib.h>
#include <glibconfig.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "audio.h"

static constexpr char CPU_FILE[] = "/proc/stat";
static constexpr char MEM_FILE[] = "/proc/meminfo";
static constexpr char NET_FILE[] = "/proc/net/dev";

static constexpr uint32_t INTERVAL = 1;
static constexpr char TIME_FORMAT[] = "%A %m-%d %H:%M:%S";

static status_t status;
static guint timer = 0;
static status_changed_notify listener = nullptr;

typedef struct cpu_stat_t {
  uint64_t user;
  uint64_t nice;
  uint64_t system;
  uint64_t idle;
  uint64_t iowait;
  uint64_t irq;
  uint64_t softirq;
  uint64_t steal;
  uint64_t guest;
  uint64_t guest_nice;
} cpu_stat_t;

static double calc_cpu_usage(cpu_stat_t curr, cpu_stat_t prev) {
  uint64_t curr_idle = curr.idle + curr.iowait;
  uint64_t curr_total = curr.user + curr.nice + curr.system + curr.idle +
                        curr.iowait + curr.irq + curr.softirq + curr.steal +
                        curr.guest + curr.guest_nice;

  uint64_t prev_idle = prev.idle + prev.iowait;
  uint64_t prev_total = prev.user + prev.nice + prev.system + prev.idle +
                        prev.iowait + prev.irq + prev.softirq + prev.steal +
                        prev.guest + prev.guest_nice;

  uint64_t total = curr_total - prev_total;
  uint64_t idle = curr_idle - prev_idle;
  uint64_t active = total - idle;
  if (total == 0) return 0.0;

  double usage = ((double)active) / total * 100;
  return usage;
}

static bool get_cpu_usage(double *usage, GError *error) {
  static cpu_stat_t prev_cpu_stat = {0};
  static bool inited = false;

  gchar *contents = nullptr;
  gsize length = 0;

  if (!g_file_get_contents(CPU_FILE, &contents, &length, &error)) return false;

  gchar **lines = g_strsplit(contents, "\n", 0);
  g_free(contents);

  char cpu_line[256];
  for (gchar **line = lines; *line != nullptr; ++line) {
    if (strncmp(cpu_line, "cpu", 4) == 0) {
      strncpy(cpu_line, *line, sizeof(cpu_line));
      break;
    }
  }

  cpu_stat_t stat = {0};
  sscanf(cpu_line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &stat.user,
         &stat.nice, &stat.system, &stat.idle, &stat.iowait, &stat.irq,
         &stat.softirq, &stat.steal, &stat.guest, &stat.guest_nice);

  if (!inited) {
    prev_cpu_stat = stat;
    inited = true;
  }

  double percent = calc_cpu_usage(stat, prev_cpu_stat);
  prev_cpu_stat = stat;
  *usage = percent;

  g_strfreev(lines);
  return true;
}

static void bytes_to_readable_size(uint64_t bytes, char *str) {
  static char *uints[] = {"B", "K", "M", "G", "T"};

  double size = (double)bytes;
  int i = 0;
  while (size > 1024.0) {
    size /= 1024.0;
    ++i;
  }
  if (i == 0) {
    sprintf(str, "%ld%s", bytes, uints[i]);
  } else {
    sprintf(str, "%.1lf%s", size, uints[i]);
  }
}

static void calc_mem_usage(memory_usage_t *usage) {
  usage->mem_used = usage->mem_total - usage->mem_free - usage->buffers -
                    usage->cached - usage->s_reclaimable;
  usage->swap_used = usage->swap_total - usage->swap_free;

  usage->mem_percent =
    ((float)usage->mem_used) / ((float)usage->mem_total) * 100;
  if (usage->swap_total == 0) {
    usage->swap_percent = 0;
  } else {
    usage->swap_percent =
      ((float)usage->swap_used) / ((float)usage->swap_total) * 100;
  }

  bytes_to_readable_size(usage->mem_used * 1024, usage->mem_used_text);
  bytes_to_readable_size(usage->swap_used * 1024, usage->swap_used_text);
}

static bool get_mem_usage(memory_usage_t *usage, GError *error) {
  gchar *contents = nullptr;
  gsize length = 0;

  if (!g_file_get_contents(MEM_FILE, &contents, &length, &error)) return false;

  gchar **lines = g_strsplit(contents, "\n", 0);
  g_free(contents);

  for (gchar **line = lines; *line != nullptr; ++line) {
    char name[32];
    uint64_t kb = 0;
    sscanf(*line, "%[^:]: %lu", name, &kb);
    if (g_str_equal(name, "MemTotal")) {
      usage->mem_total = kb;
    } else if (g_str_equal(name, "MemFree")) {
      usage->mem_free = kb;
    } else if (g_str_equal(name, "Buffers")) {
      usage->buffers = kb;
    } else if (g_str_equal(name, "Cached")) {
      usage->cached = kb;
    } else if (g_str_equal(name, "SwapTotal")) {
      usage->swap_total = kb;
    } else if (g_str_equal(name, "SwapFree")) {
      usage->swap_free = kb;
    } else if (g_str_equal(name, "SReclaimable")) {
      usage->s_reclaimable = kb;
    }
  }
  calc_mem_usage(usage);
  g_strfreev(lines);
  return true;
}

typedef struct net_stat_t {
  uint64_t rx_bytes;
  uint64_t tx_bytes;
} net_stat_t;

static void calc_speed(const uint32_t interval, const net_stat_t *current,
                       const net_stat_t *previous, net_speed_t *speed) {
  speed->rx_bytes = current->rx_bytes - previous->rx_bytes;
  speed->tx_bytes = current->tx_bytes - previous->tx_bytes;
  bytes_to_readable_size(speed->rx_bytes / interval, speed->down);
  bytes_to_readable_size(speed->tx_bytes / interval, speed->up);
}

static bool get_net_speed(const uint32_t interval, net_speed_t *speed,
                          GError *error) {
  static net_stat_t prev = {0};
  static bool inited = false;

  gchar *contents = nullptr;
  gsize length = 0;

  if (!g_file_get_contents(NET_FILE, &contents, &length, &error)) return false;

  gchar **lines = g_strsplit(contents, "\n", 0);
  g_free(contents);

  net_stat_t stat = {0};
  for (gchar **line = lines; *line != nullptr; ++line) {
    if (strchr(*line, ':') == nullptr) continue;

    char name[16];
    uint64_t rx_bytes, tx_bytes;
    sscanf(*line, " %[^:]: %lu %*u %*u %*u %*u %*u %*u %*u %lu", name,
           &rx_bytes, &tx_bytes);
    if (g_str_equal(name, "lo")) continue;

    stat.rx_bytes += rx_bytes;
    stat.tx_bytes += tx_bytes;
  }

  if (!inited) {
    inited = true;
    prev = stat;
  }

  calc_speed(interval, &stat, &prev, speed);
  prev = stat;

  g_strfreev(lines);
  return true;
}

static void get_date_time(char *time_text, size_t length) {
  time_t current_time = time(nullptr);
  struct tm *time_info = localtime(&current_time);
  strftime(time_text, length, TIME_FORMAT, time_info);
}

static inline void notify_status_change(void) {
  if (listener) listener(&status);
}

static gboolean update_status(gpointer data) {
  GError *error = nullptr;
  get_net_speed(INTERVAL, &status.net_speed, error);
  get_mem_usage(&status.mem_usage, error);
  get_cpu_usage(&status.cpu_usage_percent, error);
  get_date_time(status.time, sizeof(status.time));
  notify_status_change();

  return true;
}

void notify_pulse_change(pulse_t *pulse) {
  status.pulse = pulse;
  notify_status_change();
}

void init_status(GMainContext *context, status_changed_notify callback) {
  listener = callback;
  status.pulse_context = init_pulse(context, notify_pulse_change);
  update_status(nullptr);
  timer = g_timeout_add_seconds(INTERVAL, update_status, nullptr);
}

void clean_status(void) {
  clean_pulse(status.pulse_context);
  g_source_remove(timer);
  status.pulse_context = nullptr;
  listener = nullptr;
}
