#include "audio.h"

#include <glib.h>
#include <math.h>
#include <pulse/context.h>
#include <pulse/def.h>
#include <pulse/glib-mainloop.h>
#include <pulse/introspect.h>
#include <pulse/mainloop-api.h>
#include <pulse/proplist.h>
#include <pulse/subscribe.h>
#include <pulse/volume.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "utils.h"

struct pulse_context_t {
  GMainContext *g_context;
  pulse_notify_t callback;
  pa_glib_mainloop *loop;
  pa_context *context;
  pa_cvolume volume;
  pulse_t pulse;
  uint32_t index;
};

static void clean_pulse_context(pulse_context_t *context, bool full_cleanup);
static void init_pulse_context(pulse_context_t *context, bool create_loop);
static void state_callback(pa_context *context, void *userdata);
static void subscribe_callback(pa_context *context,
                               pa_subscription_event_type_t t, uint32_t index,
                               void *userdata);
static void server_info_callback(pa_context *context,
                                 const pa_server_info *info, void *userdata);
static void sink_info_callback(pa_context *context, const pa_sink_info *info,
                               int eol, void *userdata);
static void parse_sink(const pa_sink_info *info, pulse_t *pulse);

pulse_context_t *init_pulse(GMainContext *context, pulse_notify_t callback) {
  pulse_context_t *ctx = p_new(pulse_context_t, 1);
  ctx->g_context = context;
  ctx->callback = callback;
  init_pulse_context(ctx, true);

  return ctx;
}

void change_pulse_volume(pulse_context_t *context, int step) {
  if (!context->pulse.inited || !step || step > 100 || step < -100) return;

  pa_cvolume vol = context->volume;
  if (step > 0) {
    pa_cvolume_inc(&vol, PA_VOLUME_NORM * step / 100);
    if (pa_cvolume_max(&vol) > PA_VOLUME_NORM) {
      pa_cvolume_set(&vol, vol.channels, PA_VOLUME_NORM);
    }
  } else {
    pa_cvolume_dec(&vol, PA_VOLUME_NORM * (-step) / 100);
    if (pa_cvolume_min(&vol) < PA_VOLUME_MUTED) {
      pa_cvolume_set(&vol, vol.channels, PA_VOLUME_MUTED);
    }
  }
  pa_context *ctx = context->context;
  uint32_t index = context->index;
  pa_context_set_sink_volume_by_index(ctx, index, &vol, nullptr, nullptr);
}

void toggle_pulse_mute(pulse_context_t *context) {
  if (!context->pulse.inited) return;

  pa_context *ctx = context->context;
  uint32_t index = context->index;
  bool mute = !context->pulse.mute;
  pa_context_set_sink_mute_by_index(ctx, index, mute, nullptr, nullptr);
}

void clean_pulse(pulse_context_t *context) {
  clean_pulse_context(context, true);
  p_delete(&context);
}

void clean_pulse_context(pulse_context_t *context, bool full_cleanup) {
  memset(&context->pulse, 0, sizeof(context->pulse));
  if (context->callback) context->callback(&context->pulse);
  if (context->context) {
    pa_context_set_state_callback(context->context, nullptr, nullptr);
    pa_context_set_subscribe_callback(context->context, nullptr, nullptr);
    pa_context_disconnect(context->context);
    pa_context_unref(context->context);
    context->context = nullptr;
  }
  context->pulse.inited = false;
  if (full_cleanup && context->loop) {
    pa_glib_mainloop_free(context->loop);
    context->loop = nullptr;
  }
}

void init_pulse_context(pulse_context_t *context, bool create_loop) {
  if (create_loop) {
    if (context->loop) pa_glib_mainloop_free(context->loop);
    context->loop = pa_glib_mainloop_new(context->g_context);
  }
  pa_mainloop_api *api = pa_glib_mainloop_get_api(context->loop);
  context->context = pa_context_new(api, APP_NAME "-audio-status");
  pa_context_connect(context->context, nullptr, PA_CONTEXT_NOFAIL, nullptr);
  pa_context_set_state_callback(context->context, state_callback, context);
}

void state_callback(pa_context *context, void *userdata) {
  pulse_context_t *ctx = userdata;

  pa_context_state_t state = pa_context_get_state(context);
  if (PA_CONTEXT_IS_GOOD(state)) {
    pa_context_set_subscribe_callback(context, subscribe_callback, ctx);
    pa_context_subscribe(context, PA_SUBSCRIPTION_MASK_SINK, nullptr, nullptr);
    pa_context_get_server_info(context, server_info_callback, ctx);
  } else if (state == PA_CONTEXT_FAILED) {
    clean_pulse_context(ctx, false);
    init_pulse_context(ctx, false);
  }
}

void subscribe_callback(pa_context *context, pa_subscription_event_type_t t,
                        uint32_t index, void *userdata) {
  auto facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
  if (facility != PA_SUBSCRIPTION_EVENT_SINK) return;

  pa_subscription_event_type_t event = t & PA_SUBSCRIPTION_EVENT_TYPE_MASK;
  if (event == PA_SUBSCRIPTION_EVENT_CHANGE) {
    pa_context_get_server_info(context, server_info_callback, userdata);
  }
}

void server_info_callback(pa_context *context, const pa_server_info *info,
                          void *userdata) {
  pa_context_get_sink_info_by_name(context, info->default_sink_name,
                                   sink_info_callback, userdata);
}

void sink_info_callback(pa_context *context, const pa_sink_info *info, int eol,
                        void *userdata) {
  if (eol != 0 || !info) return;

  pulse_context_t *ctx = userdata;
  ctx->pulse.inited = true;
  ctx->volume = info->volume;
  ctx->index = info->index;
  parse_sink(info, &ctx->pulse);
  if (ctx->callback) ctx->callback(&ctx->pulse);
}

static inline int volume_to_percent(pa_volume_t volume) {
  return (int)round(((double)volume) * 100 / PA_VOLUME_NORM);
}

void parse_sink(const pa_sink_info *info, pulse_t *pulse) {
  size_t size = sizeof(pulse->device_name);
  pulse->mute = info->mute;
  pulse->volume_percent = volume_to_percent(pa_cvolume_avg(&info->volume));
  strncpy(pulse->device_name, info->description, size);

  const char *key = nullptr;
  void *state = nullptr;
  while ((key = pa_proplist_iterate(info->proplist, &state)) != nullptr) {
    if (strcmp(key, "api.alsa.path") != 0 &&
        strcmp(key, "device.description") != 0) {
      continue;
    }

    const char *value = pa_proplist_gets(info->proplist, key);
    size_t len = strnlen(value, size);
    if (len && (len < strnlen(pulse->device_name, size))) {
      strncpy(pulse->device_name, value, size);
      pulse->device_name[size - 1] = '\0';
    }
  }
}
