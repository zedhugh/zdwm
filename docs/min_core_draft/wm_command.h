#pragma once

#include "wm_types.h"

typedef struct wm_manage_window_init_t {
  wm_window_geometry_mode_t geometry_mode;
  bool floating;
  bool sticky;
  bool urgent;
  bool set_focus;
} wm_manage_window_init_t;

typedef enum wm_command_type_t {
  WM_COMMAND_NONE,
  WM_COMMAND_MANAGE_WINDOW,
  WM_COMMAND_UNMANAGE_WINDOW,
  WM_COMMAND_FOCUS_WINDOW,
  WM_COMMAND_FOCUS_DIRECTION,
  WM_COMMAND_SET_WINDOW_URGENT,
  WM_COMMAND_RAISE_WINDOW,
  WM_COMMAND_LOWER_WINDOW,
  WM_COMMAND_SWITCH_WORKSPACE,
  WM_COMMAND_SEND_WINDOW_TO_WORKSPACE,
  WM_COMMAND_SEND_WINDOW_TO_OUTPUT,
  WM_COMMAND_TOGGLE_FLOATING,
  WM_COMMAND_TOGGLE_STICKY,
  WM_COMMAND_SET_MAXIMIZED,
  WM_COMMAND_SET_FULLSCREEN,
  WM_COMMAND_SET_MINIMIZED,
  WM_COMMAND_MOVE_FLOATING_WINDOW,
  WM_COMMAND_RESIZE_FLOATING_WINDOW,
  WM_COMMAND_BEGIN_MOVE_FLOATING_INTERACTION,
  WM_COMMAND_BEGIN_RESIZE_FLOATING_INTERACTION,
  WM_COMMAND_SET_LAYOUT,
  WM_COMMAND_CYCLE_LAYOUT,
  WM_COMMAND_REDRAW,
  WM_COMMAND_QUIT,
} wm_command_type_t;

typedef struct wm_command_t {
  wm_command_type_t type;
  union {
    struct {
      wm_window_id_t window_id;
      wm_workspace_id_t workspace_id;
      wm_manage_window_init_t initial_state;
      bool has_initial_float_rect;
      wm_rect_t initial_float_rect;
    } manage_window;

    struct {
      wm_window_id_t window_id;
    } unmanage_window;

    struct {
      wm_window_id_t window_id;
    } focus_window;

    struct {
      wm_output_id_t output_id;
      wm_focus_direction_t direction;
    } focus_direction;

    struct {
      wm_window_id_t window_id;
      bool urgent;
    } set_window_urgent;

    struct {
      wm_window_id_t window_id;
    } raise_window;

    struct {
      wm_window_id_t window_id;
    } lower_window;

    struct {
      wm_output_id_t output_id;
      wm_workspace_id_t workspace_id;
    } switch_workspace;

    struct {
      wm_window_id_t window_id;
      wm_workspace_id_t workspace_id;
    } send_window_to_workspace;

    struct {
      wm_window_id_t window_id;
      wm_output_id_t output_id;
      wm_cross_output_policy_t policy;
    } send_window_to_output;

    struct {
      wm_window_id_t window_id;
    } toggle_floating;

    struct {
      wm_window_id_t window_id;
    } toggle_sticky;

    struct {
      wm_window_id_t window_id;
      bool enabled;
    } set_maximized;

    struct {
      wm_window_id_t window_id;
      bool enabled;
    } set_fullscreen;

    struct {
      wm_window_id_t window_id;
      bool enabled;
    } set_minimized;

    struct {
      wm_window_id_t window_id;
      int16_t dx;
      int16_t dy;
      bool commit_workspace_change;
      wm_point_t anchor;
      wm_cross_output_policy_t policy;
    } move_floating_window;

    struct {
      wm_window_id_t window_id;
      int16_t dw;
      int16_t dh;
    } resize_floating_window;

    struct {
      wm_window_id_t window_id;
    } begin_move_floating_interaction;

    struct {
      wm_window_id_t window_id;
    } begin_resize_floating_interaction;

    struct {
      wm_workspace_id_t workspace_id;
      wm_layout_id_t layout_id;
    } set_layout;

    struct {
      wm_workspace_id_t workspace_id;
      int direction;  // 1: 下一个布局, -1: 上一个布局
    } cycle_layout;
  } as;
} wm_command_t;
