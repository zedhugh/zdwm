#include <stdint.h>

static const char *font_family = "monospace";
static const int font_size = 10;
static const int default_dpi = 144;

static const char *const tags[] = {
  "1", "2", "3", "4", "5", "6", "7", "8", "9", nullptr,
};
static const uint16_t bar_y_padding = 1;
static const uint16_t tag_x_padding = 10;

static const char *const bar_bg = "#222222";
static const char *const tag_bg = "#222222";
static const char *const active_tag_bg = "#005577";
static const char *const tag_color = "#bbbbbb";
static const char *const active_tag_color = "#eeeeee";
