#pragma once

#include <stddef.h>
#include <zdwm/bar.h>
#include <zdwm/types.h>

size_t bar_cell_get_count(zdwm_bar_item_t *item);
void bar_cell_set_count(zdwm_bar_item_t *item, size_t count);

void bar_cell_set_text(zdwm_bar_item_t *item, size_t index, const char *text);
void bar_cell_set_bg(zdwm_bar_item_t *item, size_t index, const char *color);
void bar_cell_set_fg(zdwm_bar_item_t *item, size_t index, const char *color);
void bar_cell_set_icon(zdwm_bar_item_t *item, size_t index, zdwm_icon_t icon);
void bar_cell_set_indicator(zdwm_bar_item_t *item, size_t index, bool show);
