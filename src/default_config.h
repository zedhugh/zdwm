#pragma once

#include <X11/keysym.h>
#include <stdint.h>

#include "action.h"
#include "layout.h"
#include "types.h"

static const char *const font_family = "monospace";
static const int font_size = 10;
static const int default_dpi = 144;

static const char *const tags[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", nullptr,
};
static const uint16_t border_width = 1;
static const uint16_t bar_y_padding = 1;
static const uint16_t tag_x_padding = 10;

static const char *const bar_bg = "#222222";
static const char *const tag_bg = "#222222";
static const char *const active_tag_bg = "#005577";
static const char *const tag_color = "#bbbbbb";
static const char *const active_tag_color = "#eeeeee";
static const char *const border_color = "#444444";
static const char *const active_border_color = "#005577";

static const button_t button_list[] = {
  {click_tag, modifier_none, button_left, select_tag_of_current_monitor, {0}},
};

#define TAGKEYS(KEY, TAG)                                                  \
  {modifier_super, KEY, select_tag_of_current_monitor, {.ui = TAG}}, {     \
    modifier_super | modifier_shift, KEY, send_client_to_tag, {.ui = TAG}, \
  }

static const char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";
static const char terminal[] = "xterm";
static const char terminal_class[] = "XTerm";
static const char editor[] = "emacsclient -a '' -r";
static const char editor_class[] = "Emacs";
static const char browser[] = "firefox-bin";
static const char browser_class[] = "firefox";
static const keyboard_t key_list[] = {
  {modifier_super, XK_r, spawn, {.ptr = launcher}},
  {modifier_super, XK_Return, spawn, {.ptr = terminal}},
  {
    modifier_control | modifier_alt,
    XK_r,
    raise_or_run,
    {.ptr = (const char *[]){terminal_class, terminal}},
  },
  {
    modifier_super,
    XK_e,
    raise_or_run,
    {.ptr = (const char *[]){editor_class, editor}},
  },
  {
    modifier_super,
    XK_q,
    raise_or_run,
    {.ptr = (const char *[]){browser_class, browser}},
  },
  {modifier_super | modifier_shift, XK_q, quit, {.b = false}},
  {modifier_super | modifier_control, XK_r, quit, {.b = true}},
  {modifier_super | modifier_shift,
   XK_j,
   focus_client_in_same_tag,
   {.b = true}},
  {modifier_super | modifier_shift,
   XK_k,
   focus_client_in_same_tag,
   {.b = false}},

  TAGKEYS(XK_1, 1 << 0),
  TAGKEYS(XK_2, 1 << 1),
  TAGKEYS(XK_3, 1 << 2),
  TAGKEYS(XK_4, 1 << 3),
  TAGKEYS(XK_5, 1 << 4),
  TAGKEYS(XK_6, 1 << 5),
  TAGKEYS(XK_7, 1 << 6),
  TAGKEYS(XK_8, 1 << 7),
  TAGKEYS(XK_9, 1 << 8),

  {modifier_super, XK_o, send_client_to_next_monitor, {0}},

  {modifier_super | modifier_control, XK_space, toggle_client_floating, {0}},
  {modifier_super, XK_f, toggle_client_fullscreen, {0}},
  {modifier_super, XK_m, toggle_client_maximize, {0}},
};

static const layout_t layout_list[] = {
  {.symbol = "[]=", .arrange = tile},
  {.symbol = "[M]", .arrange = monocle},
  {.symbol = "><=", .arrange = nullptr},
};

static const char *const autostart_list[] = {
  "gentoo-pipewire-launcher restart",
  "picom -b --backend xrender",
  "fcitx5 -d",
  nullptr,
};

static const rule_t rules[] = {
  {.role = "dialog", .tag_index = -1, .floating = true},
  {.class = "firefox", .tag_index = 1, .maximize = true},
  {.class = "Google-chrome", .tag_index = 1, .maximize = true},
  {.class = "mpv", .tag_index = 3, .switch_to_tag = true, .fullscreen = true},
  {.class = "Emacs", .tag_index = 10, .switch_to_tag = true, .maximize = true},
};
