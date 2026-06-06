#include "config/defaults.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "base/log.h"
#include "base/macros.h"
#include "core/types.h"

static constexpr char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";

typedef struct zdwm_action_data_raise_or_run_t raise_or_run_data_t;

static raise_or_run_data_t terminal = {"XTerm", "xterm"};
static raise_or_run_data_t editor   = {"Emacs", "emacsclient -a '' -r -n"};
static raise_or_run_data_t browser  = {"firefox", "firefox-bin"};
static raise_or_run_data_t chrome   = {"Google-chrome", "google-chrome-stable"};

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
#define DEF_WORKSPACE(NAME) \
  api->define_workspace(    \
    builder,                \
    i,                      \
    (NAME),                 \
    layout_ids,             \
    countof(layout_ids),    \
    fair_id                 \
  )
    char *workspace_names[] = {"1 terminal", "2 Editor", "3 Browser"};
    for (size_t j = 0; j < countof(workspace_names); ++j) {
      auto workspace = DEF_WORKSPACE(workspace_names[j]);
      if (workspace_id_invalid(workspace)) {
        logger("== define workspace \"%s\" failed\n", workspace_names[j]);
        return false;
      }
    }
#undef DEF_WORKSPACE
  }

  auto default_mode = api->add_mode(builder, "default");

#define Alt_Key   "Mod1"
#define Super_key "Mod4"
#define Alt(K)    Alt_Key "+" K
#define Super(K)  Super_key "+" K

#define BIND(MODE, KEY, ...) \
  api->bind(builder, MODE, KEY, (zdwm_action_t)__VA_ARGS__)
#define SPAWN(MODE, KEY, COMMAND) \
  BIND(MODE, KEY, {.type = ZDWM_ACTION_SPAWN, .as.spawn.command = (COMMAND)})
#define QUIT(MODE, KEY, RESTART) \
  BIND(MODE, KEY, {.type = ZDWM_ACTION_QUIT, .as.quit.restart = (RESTART)})
#define RAISE_OR_RUN(MODE, KEY, ...)                                   \
  BIND(                                                                \
    MODE,                                                              \
    KEY,                                                               \
    {                                                                  \
      .type            = ZDWM_ACTION_RAISE_OR_RUN,                     \
      .as.raise_or_run = (zdwm_action_data_raise_or_run_t)__VA_ARGS__, \
    }                                                                  \
  )
#define BIND_DEFAULT(KEY, ...) BIND(default_mode, KEY, __VA_ARGS__)

  SPAWN(default_mode, Super("r"), launcher);
  QUIT(default_mode, Super("Shift+q"), false);
  QUIT(default_mode, Super("Control+r"), true);

  SPAWN(default_mode, Super("Return"), terminal.command);
  RAISE_OR_RUN(default_mode, Alt("Control+r"), terminal);
  RAISE_OR_RUN(default_mode, Super("e"), editor);
  RAISE_OR_RUN(default_mode, Super("q"), browser);
  RAISE_OR_RUN(default_mode, Super("a"), chrome);
  BIND_DEFAULT(Super("f"), {.type = ZDWM_ACTION_WINDOW_TOGGLE_FLOATING});
  BIND_DEFAULT(Super("m"), {.type = ZDWM_ACTION_WINDOW_TOGGLE_MAXIMIZE});
  BIND_DEFAULT(
    Super("Control+space"),
    {.type = ZDWM_ACTION_WINDOW_TOGGLE_FLOATING}
  );
  BIND_DEFAULT(
    Alt("j"),
    {.type = ZDWM_ACTION_WINDOW_FOCUS_CYCLE, .as.window_focus_cycle.delta = 1}
  );
  BIND_DEFAULT(
    Alt("k"),
    {.type = ZDWM_ACTION_WINDOW_FOCUS_CYCLE, .as.window_focus_cycle.delta = -1}
  );

  zdwm_action_t window_cycle_output_action = {
    .type                   = ZDWM_ACTION_WINDOW_CYCLE_OUTPUT,
    .as.window_cycle_output = {.delta = 1, .keep_focus = true},
  };
  BIND_DEFAULT(Super("o"), window_cycle_output_action);

  char buffer[64] = {0};
  for (uint32_t i = 0; i < 9; ++i) {
    snprintf(buffer, sizeof(buffer), Super("%d"), i + 1);
    zdwm_action_t switch_workspace_action = {
      .type = ZDWM_ACTION_WORKSPACE_SWITCH_SAME_OUTPUT_BY_INDEX,
      .as.workspace_switch_same_output_by_index.index = i,
    };
    BIND_DEFAULT(buffer, switch_workspace_action);

    snprintf(buffer, sizeof(buffer), Super("Shift+%d"), i + 1);
    zdwm_action_t send_window_to_workspace_action = {
      .type = ZDWM_ACTION_WINDOW_SEND_TO_WORKSPACE_SAME_OUTPUT_BY_INDEX,
      .as.window_send_to_workspace_same_output_by_index = {
        .index            = i,
        .switch_workspace = false,
      },
    };
    BIND_DEFAULT(buffer, send_window_to_workspace_action);
  }

#undef BIND_DEFAULT
#undef BIND

  api->set_default_mode(builder, default_mode);
  api->set_initial_mode(builder, default_mode);
  api->set_border_config(builder, 2, "#1c2022", "#606060");

  zdwm_bar_config_t bar_config = {
    .height      = 28,
    .show_top    = true,
    .padding_x   = 10,
    .fps         = 30,
    .font_family = "Terminus, Sarasa Term SC",
    .font_size   = 10,
    .dpi         = 144,

    .bg = "#222222",
  };
  api->set_bar_config(builder, bar_config);

  return true;
}
