#pragma once

#include <stdint.h>
#include <xcb/xproto.h>

typedef union user_action_arg_t {
  bool b;
  int32_t i;
  uint32_t ui;
  const void *ptr;
} user_action_arg_t;

typedef enum click_area_t {
  click_none,
  click_tag,
} click_area_t;

typedef enum modifier_t {
  modifier_none = 0,
  modifier_shift = XCB_MOD_MASK_SHIFT,
  modifier_control = XCB_MOD_MASK_CONTROL,
  modifier_alt = XCB_MOD_MASK_1,
  modifier_super = XCB_MOD_MASK_4,
  modifier_any = XCB_MOD_MASK_ANY,
} modifier_t;

typedef uint16_t modifier_value_t;

typedef enum button_index_t {
  button_any = XCB_BUTTON_INDEX_ANY,
  button_left = XCB_BUTTON_INDEX_1,
  button_right = XCB_BUTTON_INDEX_2,
  button_middle = XCB_BUTTON_INDEX_3,
} button_index_t;

typedef struct button_t {
  click_area_t click_area : 16;
  modifier_value_t modifiers;
  button_index_t button;
  void (*func)(const user_action_arg_t *arg);
  const user_action_arg_t arg;
} button_t;

typedef struct keyboard_t {
  modifier_value_t modifiers;
  xcb_keysym_t keysym;
  void (*func)(const user_action_arg_t *arg);
  const user_action_arg_t arg;
} keyboard_t;

void focus_client_in_same_tag(const user_action_arg_t *arg);
void select_tag_of_current_monitor(const user_action_arg_t *arg);
void send_client_to_tag(const user_action_arg_t *arg);
void spawn(const user_action_arg_t *arg);
void quit(const user_action_arg_t *arg);
