#include "config/defaults.h"

#include <zdwm/action.h>
#include <zdwm/types.h>

#include "base/macros.h"

static constexpr char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";

typedef struct raise_or_run_arg_t {
  const char *class_name;
  const char *command;
} raise_or_run_arg_t;

static raise_or_run_arg_t terminal = {"XTerm", "xterm"};
static raise_or_run_arg_t editor   = {"Emacs", "emacsclient -a '' -r -n"};
static raise_or_run_arg_t browser  = {"firefox", "firefox-bin"};
static raise_or_run_arg_t chrome   = {"Google-chrome", "google-chrome-stable"};

static void spawn(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->spawn(arg->str);
}

static void quit(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->quit(runtime, arg->b);
}

static void raise_or_run(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  auto args = (raise_or_run_arg_t *)arg->ptr;
  api->raise_or_run(runtime, args->class_name, args->command);
}

static void toggle_fullscreen(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->toggle_fullscreen(runtime);
}

static void toggle_maximize(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->toggle_maximize(runtime);
}

static void toggle_floating(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->toggle_floating(runtime);
}

static void focus_window(
  zdwm_runtime_t *runtime,
  const zdwm_action_api_t *api,
  const zdwm_action_arg_t *arg
) {
  api->focus_window(runtime, arg->i);
}

bool config_defaults_build(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  const zdwm_output_info_t *outputs,
  size_t output_count
) {
  if (!api || !builder || !outputs || output_count == 0) return false;

  zdwm_layout_id_t fair_id = api->register_layout(
    builder,
    "fair",
    "[]=",
    "builtin fair",
    api->builtin_layouts.fair
  );
  zdwm_layout_id_t maximize_id = api->register_layout(
    builder,
    "maximize",
    "[M]",
    "builtin maximize",
    api->builtin_layouts.maximize
  );
  zdwm_layout_id_t fullscreen_id = api->register_layout(
    builder,
    "fullscreen",
    "[F]",
    "builtin fullscreen",
    api->builtin_layouts.fullscreen
  );
  zdwm_layout_id_t floating_id =
    api->register_layout(builder, "floating", "><>", "floating", nullptr);
  if (fair_id == ZDWM_LAYOUT_ID_INVALID ||
      maximize_id == ZDWM_LAYOUT_ID_INVALID ||
      fullscreen_id == ZDWM_LAYOUT_ID_INVALID ||
      floating_id == ZDWM_LAYOUT_ID_INVALID) {
    return false;
  }

  zdwm_layout_id_t layout_ids[] = {
    fair_id,
    maximize_id,
    fullscreen_id,
    floating_id,
  };
  for (size_t i = 0; i < output_count; i++) {
    if (api->define_workspace(
          builder,
          i,
          "main",
          layout_ids,
          countof(layout_ids),
          fair_id
        ) == ZDWM_WORKSPACE_ID_INVALID) {
      return false;
    }
  }

  auto default_mode = api->add_mode(builder, "default");

#define Alt   "Mod1"
#define Super "Mod4"
#define BIND(MODE, KEY, FN, ARG) \
  api->bind(builder, MODE, KEY, FN, (zdwm_action_arg_t)ARG)

  BIND(default_mode, Super "+r", spawn, {.str = launcher});
  BIND(default_mode, Super "+Shift+q", quit, {.b = false});
  BIND(default_mode, Super "+Control+r", quit, {.b = true});

  BIND(default_mode, Super "+Return", spawn, {.str = terminal.command});
  BIND(default_mode, Alt "+Control+r", raise_or_run, {.ptr = &terminal});
  BIND(default_mode, Super "+e", raise_or_run, {.ptr = &editor});
  BIND(default_mode, Super "+q", raise_or_run, {.ptr = &browser});
  BIND(default_mode, Super "+a", raise_or_run, {.ptr = &chrome});
  BIND(default_mode, Super "+f", toggle_fullscreen, {0});
  BIND(default_mode, Super "+m", toggle_maximize, {0});
  BIND(default_mode, Super "+Control+space", toggle_floating, {0});
  BIND(default_mode, Alt "+j", focus_window, {.i = 1});
  BIND(default_mode, Alt "+k", focus_window, {.i = -1});

#undef BIND

  api->set_default_mode(builder, default_mode);
  api->set_initial_mode(builder, default_mode);
  api->set_border_config(builder, 2, "#444444", "#005577");

  return true;
}
