#include "config/defaults.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <zdwm/action.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

#include "base/log.h"
#include "base/macros.h"
#include "interface/types.h"

static constexpr char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";

typedef struct zdwm_action_data_raise_or_run_t raise_or_run_data_t;

static raise_or_run_data_t terminal = {"XTerm", "xterm"};
static raise_or_run_data_t editor = {"Emacs", "emacsclient -a '' -r -n"};
static raise_or_run_data_t browser = {"firefox", "firefox-bin"};
static raise_or_run_data_t chrome = {"Google-chrome", "google-chrome-stable"};

typedef struct layouts_t {
  zdwm_layout_id_t ids[4];
  zdwm_layout_id_t fair;
  zdwm_layout_id_t floating;
} layouts_t;

static struct {
  workspace_id_t browser;
  workspace_id_t emacs;
  workspace_id_t media;
} special_workspaces = {
  .browser = ZDWM_WORKSPACE_ID_INVALID,
  .emacs = ZDWM_WORKSPACE_ID_INVALID,
  .media = ZDWM_WORKSPACE_ID_INVALID,
};

static bool register_layouts(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  layouts_t *layouts
) {
  auto builtin_layouts = &api->builtin_layouts;
  const struct {
    const char *name;
    const char *symbol;
    const char *description;
    zdwm_layout_fn fn;
  } defs[] = {
    {"fair", "[]=", "builtin fair", builtin_layouts->fair},
    {"maximize", "[M]", "builtin maximize", builtin_layouts->maximize},
    {"fullscreen", "[F]", "builtin fullscreen", builtin_layouts->fullscreen},
    {"floating", "><>", "floating", nullptr},
  };

  for (size_t i = 0; i < countof(defs); ++i) {
    auto def = &defs[i];
    auto name = def->name;
    auto symbol = def->symbol;
    auto description = def->description;
    auto fn = def->fn;
    auto id = api->register_layout(builder, name, symbol, description, fn);
    if (id == ZDWM_LAYOUT_ID_INVALID) {
      logger("== register layout \"%s\" failed\n", name);
      return false;
    }
  }
  layouts->fair = layouts->ids[0];
  layouts->floating = layouts->ids[3];
  return true;
}

static bool define_workspace(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  size_t output_count,
  const layouts_t *layouts
) {
  typedef struct workspace_config_t {
    const char *name;
    zdwm_layout_id_t default_layout;
    zdwm_workspace_id_t *slot;
  } workspace_config_t;

#define DEF_WORKSPACE(OUTPUT_INDEX, WORKSPACE_CONFIG_LIST)         \
  do {                                                             \
    auto config = &(WORKSPACE_CONFIG_LIST)[i];                     \
    auto workspace = api->define_workspace(                        \
      builder,                                                     \
      (OUTPUT_INDEX),                                              \
      config->name,                                                \
      layouts->ids,                                                \
      countof(layouts->ids),                                       \
      config->default_layout                                       \
    );                                                             \
    if (workspace_id_invalid(workspace)) {                         \
      logger("== define workspace \"%s\" failed\n", config->name); \
      return false;                                                \
    }                                                              \
    if (config->slot != nullptr) *config->slot = workspace;        \
  } while (false)

  if (output_count == 2) {
    const workspace_config_t right[] = {
      {"1 ", layouts->fair, nullptr},
      {"2 ", layouts->floating, &special_workspaces.browser},
      {"3 ", layouts->fair, nullptr}
    };
    const workspace_config_t left[] = {
      {"1 ", layouts->fair, nullptr},
      {"2 ", layouts->fair, &special_workspaces.emacs},
      {"3 ", layouts->fair, &special_workspaces.media},
      {"5 ", layouts->fair, nullptr},
    };

    for (size_t i = 0; i < countof(right); ++i) DEF_WORKSPACE(0, right);
    for (size_t i = 0; i < countof(left); ++i) DEF_WORKSPACE(1, left);
  } else {
    const workspace_config_t list[] = {
      {"1 ", layouts->fair, nullptr},
      {"2 ", layouts->floating, &special_workspaces.browser},
      {"3 ", layouts->fair, &special_workspaces.emacs},
      {"4 ", layouts->fair, &special_workspaces.media},
      {"5 ", layouts->fair, nullptr},
    };
    for (size_t i = 0; i < countof(list); ++i) DEF_WORKSPACE(0, list);
  }

#undef DEF_WORKSPACE
  return true;
}

static bool bind_default_keys(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  zdwm_binding_mode_id_t mode
) {
#define Alt_Key   "Mod1"
#define Super_key "Mod4"
#define Alt(K)    Alt_Key "+" K
#define Super(K)  Super_key "+" K

#define BIND(KEY, ...)                                                  \
  do {                                                                  \
    if (!api->bind(builder, mode, (KEY), (zdwm_action_t)__VA_ARGS__)) { \
      logger("== bind \"%s\" failed\n", (KEY));                         \
      return false;                                                     \
    }                                                                   \
  } while (false)
#define SPAWN(KEY, COMMAND) \
  BIND(KEY, {.type = ZDWM_ACTION_SPAWN, .as.spawn.command = (COMMAND)})
#define QUIT(KEY, RESTART) \
  BIND(KEY, {.type = ZDWM_ACTION_QUIT, .as.quit.restart = (RESTART)})
#define RAISE_OR_RUN(KEY, ...)                                         \
  BIND(                                                                \
    KEY,                                                               \
    {                                                                  \
      .type = ZDWM_ACTION_RAISE_OR_RUN,                                \
      .as.raise_or_run = (zdwm_action_data_raise_or_run_t)__VA_ARGS__, \
    }                                                                  \
  )

  SPAWN(Super("r"), launcher);
  QUIT(Super("Shift+q"), false);
  QUIT(Super("Control+r"), true);

  SPAWN(Super("Return"), terminal.command);
  RAISE_OR_RUN(Alt("Control+r"), terminal);
  RAISE_OR_RUN(Super("e"), editor);
  RAISE_OR_RUN(Super("q"), browser);
  RAISE_OR_RUN(Super("a"), chrome);

  zdwm_action_t window_cycle_output_action = {
    .type = ZDWM_ACTION_WINDOW_CYCLE_OUTPUT,
    .as.window_cycle_output = {.delta = 1, .keep_focus = true},
  };
  zdwm_action_t focus_next_window_action = {
    .type = ZDWM_ACTION_WINDOW_FOCUS_CYCLE,
    .as.window_focus_cycle.delta = 1
  };
  zdwm_action_t focus_previous_window_action = {
    .type = ZDWM_ACTION_WINDOW_FOCUS_CYCLE,
    .as.window_focus_cycle.delta = -1
  };
  BIND(Super("f"), {.type = ZDWM_ACTION_WINDOW_TOGGLE_FULLSCREEN});
  BIND(Super("m"), {.type = ZDWM_ACTION_WINDOW_TOGGLE_MAXIMIZE});
  BIND(Super("Control+space"), {.type = ZDWM_ACTION_WINDOW_TOGGLE_FLOATING});
  BIND(Alt("j"), focus_next_window_action);
  BIND(Alt("k"), focus_previous_window_action);
  BIND(Super("o"), window_cycle_output_action);

  char buffer[64] = {0};
  for (uint32_t i = 0; i < 9; ++i) {
    snprintf(buffer, sizeof(buffer), Super("%d"), i + 1);
    zdwm_action_t switch_workspace_action = {
      .type = ZDWM_ACTION_WORKSPACE_SWITCH_SAME_OUTPUT_BY_INDEX,
      .as.workspace_switch_same_output_by_index.index = i,
    };
    BIND(buffer, switch_workspace_action);

    snprintf(buffer, sizeof(buffer), Super("Shift+%d"), i + 1);
    zdwm_action_t send_window_to_workspace_action = {
      .type = ZDWM_ACTION_WINDOW_SEND_TO_WORKSPACE_SAME_OUTPUT_BY_INDEX,
      .as.window_send_to_workspace_same_output_by_index = {
        .index = i,
        .switch_workspace = false,
      },
    };
    BIND(buffer, send_window_to_workspace_action);
  }

#undef BIND
  return true;
}

static void
apply_default_settings(const zdwm_api_t *api, zdwm_config_builder_t *builder) {
  zdwm_bar_config_t bar_config = {
    .height = 28,
    .show_top = true,
    .padding_x = 10,
    .fps = 30,
    .font_family = "Sarasa Term SC",
    .font_size = 10,
    .dpi = 144,

    .bg = "#222222",
    .fg = "#bbbbbb",

    .tag_cell_padding_x = 10,
    .tag_indicator_width = 4,
    .tag_bg = "#222222",
    .tag_fg = "#bbbbbb",
    .tag_active_bg = "#005577",
    .tag_active_fg = "#eeeeee",
    .tag_urgent_bg = "#222222",
    .tag_urgent_fg = "#ff0000",
    .layout_bg = "#222222",
    .layout_fg = "#bbbbbb",

    .binding_show_default = true,
    .binding_padding_x = 2,
    .binding_mode_bg = "#900000",
    .binding_mode_fg = "#ffffff",

    .window_focused_bg = "#005577",
    .window_focused_fg = "#eeeeee",
  };

  api->set_border_config(builder, 2, "#1c2022", "#606060");
  api->set_interaction_fps(builder, 30);
  api->set_bar_config(builder, bar_config);
}

bool config_defaults_build(
  const zdwm_api_t *api,
  zdwm_config_builder_t *builder,
  const zdwm_output_info_t *outputs,
  size_t output_count
) {
  if (!api || !builder || !outputs || output_count == 0) return false;

  layouts_t layouts = {};
  if (!register_layouts(api, builder, &layouts)) return false;
  if (!define_workspace(api, builder, output_count, &layouts)) return false;

  auto default_mode = api->add_mode(builder, "default");
  if (default_mode == ZDWM_BINDING_MODE_ID_INVALID) {
    logger("== add mode \"default\" failed\n");
    return false;
  }
  if (!bind_default_keys(api, builder, default_mode)) return false;
  api->set_default_mode(builder, default_mode);
  api->set_initial_mode(builder, default_mode);

  apply_default_settings(api, builder);

  return true;
}
