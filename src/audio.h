#pragma once

#include <glib.h>
#include <stdint.h>

typedef struct pulse_context_t pulse_context_t;

typedef struct pulse_t {
  int volume_percent;
  bool mute;
  bool inited;
  char device_name[254];
} pulse_t;

typedef void (*pulse_notify_t)(pulse_t *pulse);

pulse_context_t *init_pulse(GMainContext *context, pulse_notify_t callback);
void change_pulse_volume(pulse_context_t *context, int step);
void toggle_pulse_mute(pulse_context_t *context);
void clean_pulse(pulse_context_t *context);
