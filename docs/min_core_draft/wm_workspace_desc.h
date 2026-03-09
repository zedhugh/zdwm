#pragma once

#include "wm_types.h"

typedef struct wm_workspace_desc_t {
  wm_workspace_id_t id;

  /*
   * 人类可读名称，例如 "web"、"code"、"chat"。
   */
  const char *name;

  /*
   * bar 中展示的文本符号，例如 "1"、"2"、"M"。
   */
  const char *symbol_text;

  /*
   * 可选图标路径。若为空，则使用 symbol_text。
   */
  const char *symbol_icon_path;
} wm_workspace_desc_t;

typedef struct wm_workspace_desc_table_t {
  wm_workspace_desc_t *items;
  size_t count;
  size_t capacity;
} wm_workspace_desc_table_t;

void wm_workspace_desc_table_init(wm_workspace_desc_table_t *table);
void wm_workspace_desc_table_shutdown(wm_workspace_desc_table_t *table);

const wm_workspace_desc_t *wm_workspace_desc_lookup(
  const wm_workspace_desc_table_t *table, wm_workspace_id_t id);

bool wm_workspace_desc_register(wm_workspace_desc_table_t *table,
                                wm_workspace_desc_t desc);
