#pragma once

#include <stdint.h>

#include "color.h"
#include "types.h"

void client_manage(xcb_window_t window);

void client_send_to_tag(client_t *client, uint32_t tag_mask);
void client_send_to_monitor(client_t *client, monitor_t *monitor);

void client_move_to(client_t *client, int16_t x, int16_t y);
void client_resize(client_t *client, uint16_t width, uint16_t height);
void client_change_border_color(client_t *client, color_t *color);
void client_change_border_width(client_t *client, uint16_t border_width);

void client_set_floating(client_t *client, bool floating);
void client_set_fullscreen(client_t *client, bool fullscreen);
void client_set_maximize(client_t *client, bool maximize);
void client_set_minimize(client_t *client, bool minimize);
void client_set_sticky(client_t *client, bool sticky);
void client_set_urgent(client_t *client, bool urgent);

void client_set_class_instance(client_t *client, const char *class,
                               const char *instance);

void client_set_name(client_t *client, char *name);
void client_set_icon_name(client_t *client, char *icon_name);
void client_set_net_name(client_t *client, char *name);
void client_set_net_icon_name(client_t *client, char *icon_name);

void client_set_role(client_t *client, char *role);

void client_set_transient_for_window(client_t *client,
                                     xcb_window_t transient_for_window);
void client_set_leader_window(client_t *client, xcb_window_t leader_window);

void client_kill(client_t *client);
void client_unmanage(client_t *client);

void client_focus(client_t *client);
void client_stack_raise(client_t *client);

bool client_is_visible(client_t *client);
