#pragma once

#include <stddef.h>
#include <xcb/xproto.h>

#include "core/backend.h"
#include "core/window.h"

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
void window_get_class(backend_t *backend, xcb_window_t window, char **class_out,
                      char **instance_out);

xcb_window_t window_get_transient_for(backend_t *backend, xcb_window_t window);

xcb_get_window_attributes_reply_t *window_get_attributes(backend_t *backend,
                                                         xcb_window_t window);
bool window_get_skip_taskbar(backend_t *backend, xcb_window_t window);
bool window_get_urgency(backend_t *backend, xcb_window_t window);
bool window_get_types(backend_t *backend, xcb_window_t window,
                      window_type_t **types, size_t *count);
bool window_get_states(backend_t *backend, xcb_window_t window,
                       window_state_t **states, size_t *count);
bool window_get_fixed_size(backend_t *backend, xcb_window_t window);
bool window_get_geometry(backend_t *backend, xcb_window_t window, rect_t *out);
window_geometry_mode_t window_get_geometry_mode(backend_t *backend,
                                                xcb_window_t window);
