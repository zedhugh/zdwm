#include "core/event.h"

#include "base/memory.h"
#include "core/window.h"

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
