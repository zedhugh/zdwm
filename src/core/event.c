#include "core/event.h"

#include "base/memory.h"

static void window_layer_props_cleanup(window_layer_props_t *props) {
  if (!props) return;

  p_delete(&props->types);
  props->type_count = 0;
  p_delete(&props->states);
  props->state_count = 0;
}

static void window_metadata_cleanup(window_metadata_t *metadata) {
  if (!metadata) return;

  p_delete(&metadata->title);
  p_delete(&metadata->app_id);
  p_delete(&metadata->role);
  p_delete(&metadata->class_name);
  p_delete(&metadata->instance_name);
}

void event_cleanup(event_t *event) {
  if (!event) return;

  switch (event->type) {
    case ZDWM_EVENT_WINDOW_MAP_REQUEST:
      window_layer_props_cleanup(&event->as.window_map_request.props);
      window_metadata_cleanup(&event->as.window_map_request.metadata);
      break;
    case ZDWM_EVENT_WINDOW_METADATA_CHANGED:
      window_metadata_cleanup(&event->as.window_metadata_change.metadata);
      break;
    default:
      break;
  }
}

void event_reset(event_t *event) {
  if (!event) return;

  event_cleanup(event);
  p_clear(event, 1);
}
