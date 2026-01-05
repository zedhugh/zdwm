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

static const char launcher[] =
  "rofi -show combi -modes combi -combi-modes window,drun,run,ssh,windowcd";
static const keyboard_t key_list[] = {
  {modifier_alt, XK_p, spawn, {.ptr = launcher}},
  {modifier_alt, XK_q, quit, {.b = false}},
  {modifier_alt, XK_r, quit, {.b = true}},
  {modifier_alt | modifier_shift, XK_j, focus_client_in_same_tag, {.b = true}},
  {modifier_alt | modifier_shift, XK_k, focus_client_in_same_tag, {.b = false}},
};

static const layout_t layout_list[] = {
  {.symbol = "[]=", .arrange = tile},
  {.symbol = "[M]", .arrange = monocle},
  {.symbol = "><=", .arrange = nullptr},
};
