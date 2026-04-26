#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>

#include "core/backend.h"
#include "core/plan.h"
#include "internal.h"

typedef struct atoms_t atoms_t;
typedef struct window_list_t window_list_t;

/**
 * @brief 获取窗口的 role 属性
 * @param backend 后端实例指针
 * @param window 目标窗口
 * @return 返回窗口的 role 属性字符串，调用者持有其内存并负责调用 free 释放
 */
char *window_get_role(backend_t *backend, xcb_window_t window);

/**
 * @brief 获取窗口标题
 * @details 获取窗口的 _NET_WM_NAME/WM_NAME 属性作为其 title
 * @param backend 后端实例指针
 * @param window 目标窗口
 * @return 返回窗口标题字符串，由调用者持有其内存并负责调用 free 释放
 */
char *window_get_title(backend_t *backend, xcb_window_t window);

/**
 * @brief 获取窗口的 class 和 instance 信息
 * @param backend 后端实例指针
 * @param window 目标窗口
 * @param class_out class 信息返回指针，调用者持有其内存并负责调用 free 释放
 * @param instance_out instance 信息返回指针，调用者持有其内存并负责调用 free
 * 释放
 */
void window_get_class(
  backend_t *backend,
  xcb_window_t window,
  char **class_out,
  char **instance_out
);
xcb_window_t window_get_transient_for(backend_t *backend, xcb_window_t window);
xcb_get_window_attributes_reply_t *
window_get_attributes(backend_t *backend, xcb_window_t window);
bool window_get_types(
  backend_t *backend,
  xcb_window_t window,
  window_type_t **types,
  size_t *count
);
bool window_get_fixed_size(backend_t *backend, xcb_window_t window, bool *out);
bool window_get_geometry(backend_t *backend, xcb_window_t window, rect_t *out);
bool window_get_atom_array(
  backend_t *backend,
  xcb_window_t window,
  xcb_atom_t property,
  xcb_atom_t **out_atoms,
  uint32_t *out_len
);
bool window_get_wm_hints(
  backend_t *backend,
  xcb_window_t window,
  xcb_icccm_wm_hints_t *out
);
window_state_t atom_to_window_state(const atoms_t *atoms, xcb_atom_t atom);

#define window_set_name_static(conn, win, name) \
  xcb_icccm_set_wm_name(conn, win, XCB_ATOM_STRING, 8, sizeof(name) - 1, name)

#define window_set_class_instance(conn, win) \
  window_set_class_instance_static(conn, win, "zdwm", "zdwm")
#define window_set_class_instance_static(conn, win, instance, class) \
  _window_set_class_instance_static(conn, win, instance "\0" class)
#define _window_set_class_instance_static(conn, win, instance_class) \
  xcb_icccm_set_wm_class(conn, win, sizeof(instance_class), instance_class)

void window_takefocus(backend_t *backend, xcb_window_t window);
void window_kill(backend_t *backend, xcb_window_t window);

void root_set_event_mask(backend_t *backend);
void window_set_event_mask(xcb_connection_t *conn, xcb_window_t window);
void window_clean_event_mask(xcb_connection_t *conn, xcb_window_t window);

void window_list_push(window_list_t *window_list, xcb_window_t window);
void window_list_reset(window_list_t *window_list);

void window_grab_keys(
  backend_t *backend,
  xcb_window_t window,
  const key_bind_t *keys,
  size_t count
);
