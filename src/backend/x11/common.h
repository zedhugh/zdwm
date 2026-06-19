#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

#include "core/types.h"

modifier_mask_t modifiers_xcb_to_zdwm(uint16_t mask);
uint16_t modifiers_zdwm_to_xcb(modifier_mask_t mask);

button_t button_xcb_to_zdwm(xcb_button_t button);
xcb_button_t button_zdwm_to_xcb(button_t button);
