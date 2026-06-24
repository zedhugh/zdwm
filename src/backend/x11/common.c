#include "common.h"

#include <stddef.h>
#include <stdint.h>
#include <xcb/xproto.h>

#include "base/macros.h"
#include "interface/types.h"

typedef struct modifier_map_t {
  modifier_bit_t modifier;
  xcb_mod_mask_t mask;
} modifier_map_t;

static constexpr modifier_map_t modifier_map[] = {
  {ZDWM_MOD_SHIFT, XCB_MOD_MASK_SHIFT},
  {ZDWM_MOD_CONTROL, XCB_MOD_MASK_CONTROL},
  {ZDWM_MOD_1, XCB_MOD_MASK_1},
  {ZDWM_MOD_2, XCB_MOD_MASK_2},
  {ZDWM_MOD_3, XCB_MOD_MASK_3},
  {ZDWM_MOD_4, XCB_MOD_MASK_4},
  {ZDWM_MOD_5, XCB_MOD_MASK_5},
};

modifier_mask_t modifiers_xcb_to_zdwm(uint16_t mask) {
  modifier_mask_t mods = ZDWM_MOD_NONE;
  for (size_t i = 0; i < countof(modifier_map); ++i) {
    auto item = &modifier_map[i];
    if (mask & item->mask) mods |= item->modifier;
  }
  return mods;
}

uint16_t modifiers_zdwm_to_xcb(modifier_mask_t mask) {
  uint16_t mods = 0U;
  for (size_t i = 0; i < countof(modifier_map); ++i) {
    auto item = &modifier_map[i];
    if (mask & item->modifier) mods |= item->mask;
  }
  return mods;
}

typedef struct button_map_t {
  button_t zdwm_button;
  xcb_button_index_t xcb_button;
} button_map_t;

static constexpr button_map_t button_map[] = {
  {ZDWM_BUTTON_LEFT, XCB_BUTTON_INDEX_1},
  {ZDWM_BUTTON_RIGHT, XCB_BUTTON_INDEX_3},
  {ZDWM_BUTTON_MIDDLE, XCB_BUTTON_INDEX_2},
  /* TODO: 后续可以考虑加上鼠标滚轮事件 */
};

button_t button_xcb_to_zdwm(xcb_button_t button) {
  for (size_t i = 0; i < countof(button_map); ++i) {
    auto item = &button_map[i];
    if (button == item->xcb_button) return item->zdwm_button;
  }

  return ZDWM_BUTTON_NONE;
}

xcb_button_t button_zdwm_to_xcb(button_t button) {
  for (size_t i = 0; i < countof(button_map); ++i) {
    auto item = &button_map[i];
    if (button == item->zdwm_button) return item->xcb_button;
  }

  return XCB_BUTTON_INDEX_ANY;
}
