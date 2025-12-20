#include "xres.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <xcb/xcb_xrm.h>

#include "wm.h"

void xres_init_xrm_db(void) {
  if (wm.xrm) return;
  wm.xrm = xcb_xrm_database_from_default(wm.xcb_conn);
}

void xres_clean(void) {
  if (!wm.xrm) return;

  xcb_xrm_database_free(wm.xrm);
  wm.xrm = nullptr;
}

/**
 * @brief 获取 Xresources 文件中配置的 long 类型配置并将其转化为 uint32_t 类型
 * @param name Xresources 文件中的配置名
 * @param value 获取到的结果存放的指针
 * @returns 配置获取是否成功，成功返回 true 失败返回 false
 */
bool xres_get_uint32(const char *name, uint32_t *value) {
  if (!wm.xrm) return false;

  long temp_value = 0;
  bool success =
    xcb_xrm_resource_get_long(wm.xrm, name, nullptr, &temp_value) == 0;
  if (success) *value = (uint32_t)temp_value;

  return success;
}

/**
 * @brief 读取 Xresources 文件中的字符串配置
 * @param name Xresources 文件中的配置名
 * @param value 存放读取到的字符串存储的指针的指针，使用完后记得使用
 * free(*value) 释放字符串对应的内存
 * @param fallback 若 Xresources 解析失败，则使用这个默认值
 */
void xres_get_string(const char *name, char **value, const char *fallback) {
  if (!wm.xrm) return;

  if (!xcb_xrm_resource_get_string(wm.xrm, name, nullptr, value)) return;

  *value = strdup(fallback);
}
