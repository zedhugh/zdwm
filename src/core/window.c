#include "core/window.h"

#include "base/memory.h"

window_layer_type_t window_classify_layer(const window_layer_props_t *props) {
  for (size_t i = 0; i < props->type_count; ++i) {
    window_type_t type = props->types[i];
    if (type == ZDWM_WINDOW_TYPE_NOTIFICATION) return ZDWM_WINDOW_LAYER_OVERLAY;
  }

  for (size_t i = 0; i < props->state_count; ++i) {
    window_state_t state = props->states[i];
    if (state == ZDWM_WINDOW_STATE_ABOVE) return ZDWM_WINDOW_LAYER_TOP;
  }

  return ZDWM_WINDOW_LAYER_NORMAL;
}

void window_layer_props_cleanup(window_layer_props_t *props) {
  if (!props) return;

  p_delete(&props->types);
  props->type_count = 0;
  p_delete(&props->states);
  props->state_count = 0;
}

void window_metadata_cleanup(window_metadata_t *metadata) {
  if (!metadata) return;

  p_delete(&metadata->title);
  p_delete(&metadata->app_id);
  p_delete(&metadata->role);
  p_delete(&metadata->class_name);
  p_delete(&metadata->instance_name);
}

void window_set_geometry_mode(
  window_t *window,
  window_geometry_mode_t geometry_mode
) {
  window->geometry_mode = geometry_mode;
}

void window_set_floating(window_t *window, bool floating) {
  if (!floating && (window->fixed_size || window->sticky)) return;

  window->floating = floating;
  if (floating) window->float_rect = window->frame_rect;
}

void window_set_sticky(window_t *window, bool sticky) {
  window->sticky = sticky;
  if (sticky) window_set_floating(window, true);
}

void window_set_urgent(window_t *window, bool urgent) {
  window->urgent = urgent;
}

void window_set_fixed_size(window_t *window, bool fixed_size) {
  window->fixed_size = fixed_size;
  if (fixed_size) window_set_floating(window, true);
}

void window_set_float_rect(window_t *window, rect_t rect) {
  window->float_rect = rect;
}

void window_set_frame_rect(window_t *window, rect_t rect) {
  window->frame_rect = rect;
}

void window_set_title(window_t *window, const char *title) {
  p_delete(&window->title);
  window->title = p_strdup_nullable(title);
}

void window_set_app_id(window_t *window, const char *app_id) {
  p_delete(&window->app_id);
  window->app_id = p_strdup_nullable(app_id);
}

void window_set_role(window_t *window, const char *role) {
  p_delete(&window->role);
  window->role = p_strdup_nullable(role);
}

void window_set_class(window_t *window, const char *class_name) {
  p_delete(&window->class_name);
  window->class_name = p_strdup_nullable(class_name);
}

void window_set_instance(window_t *window, const char *instance_name) {
  p_delete(&window->instance_name);
  window->instance_name = p_strdup_nullable(instance_name);
}

void window_set_skip_taskbar(window_t *window, bool skip_taskbar) {
  window->skip_taskbar = skip_taskbar;
}

void window_take_metadata(
  window_t *window,
  window_metadata_t *metadata,
  uint32_t changed_fields
) {
  if (changed_fields & ZDWM_WINDOW_METADATA_CHANGE_TITLE) {
    p_take(&window->title, &metadata->title);
  }
  if (changed_fields & ZDWM_WINDOW_METADATA_CHANGE_APP_ID) {
    p_take(&window->app_id, &metadata->app_id);
  }
  if (changed_fields & ZDWM_WINDOW_METADATA_CHANGE_ROLE) {
    p_take(&window->role, &metadata->role);
  }
  if (changed_fields & ZDWM_WINDOW_METADATA_CHANGE_CLASS) {
    p_take(&window->class_name, &metadata->class_name);
  }
  if (changed_fields & ZDWM_WINDOW_METADATA_CHANGE_INSTANCE) {
    p_take(&window->instance_name, &metadata->instance_name);
  }
}

bool window_need_layout(const window_t *window) {
  if (window->floating) return false;

  switch (window->geometry_mode) {
  case ZDWM_GEOMETRY_FULLSCREEN:
  case ZDWM_GEOMETRY_MAXIMIZED:
  case ZDWM_GEOMETRY_MINIMIZED:
    return false;
  default:
  }

  return true;
}
