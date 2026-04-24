#include "core/binding.h"

#include <stddef.h>
#include <stdint.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <zdwm/action.h>
#include <zdwm/types.h>

#include "base/array.h"
#include "base/memory.h"
#include "core/event.h"

typedef struct key_binding_t {
  const char *key_str;
  modifier_mask_t modifiers;
  xkb_keysym_t keysym;
  zdwm_action_fn *fn;
  zdwm_action_arg_t *arg;
} key_binding_t;

typedef struct binding_mode_t {
  zdwm_binding_mode_id_t id;
  const char *name;
  key_binding_t *items;
  size_t count;
  size_t capacity;
} binding_mode_t;

typedef struct binding_table_t {
  binding_mode_t *modes;
  size_t count;
  size_t capacity;
  zdwm_binding_mode_id_t default_mode;
  zdwm_binding_mode_id_t current_mode;
} binding_table_t;

zdwm_binding_mode_id_t
binding_table_add_mode(binding_table_t *table, const char *mode_name) {
  if (!table || !mode_name || strlen(mode_name) == 0) {
    return ZDWM_BINDING_MODE_ID_INVALID;
  }

  for (size_t i = 0; i < table->count; ++i) {
    if (strcmp(table->modes[i].name, mode_name) == 0) {
      return ZDWM_BINDING_MODE_ID_INVALID;
    }
  }

  zdwm_binding_mode_id_t id = (zdwm_binding_mode_id_t)table->count;
  binding_mode_t *new_mode =
    array_push(table->modes, table->count, table->capacity);
  new_mode->name = mode_name;
  new_mode->id   = id;

  return id;
}

static binding_mode_t *
binding_table_get_mode(binding_table_t *table, zdwm_binding_mode_id_t mode_id) {
  if (mode_id >= table->count) return nullptr;

  binding_mode_t *mode = &table->modes[mode_id];
  if (mode->id == mode_id) return mode;

  for (size_t i = 0; i < table->count; ++i) {
    mode = &table->modes[i];
    if (mode->id == mode_id) return mode;
  }

  return nullptr;
}

/**
 * @brief 解析按键序列
 *
 * @details 按键序列不合法解析失败，解析成功会设置 modifiers 与 keysym 对应的值
 *
 * @param key_sequence  按键序列
 * @param modifiers     返回修饰键的指针
 * @param keysym        返回按键符号的指针
 *
 * @return 解析成功返回 true 否则返回 false
 */
bool parse_key_sequence(
  const char *key_sequence,
  modifier_mask_t *modifiers,
  xkb_keysym_t *keysym
) {
  /* TODO: */
  return false;
}

bool binding_table_add_bind(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id,
  const char *key_sequence,
  zdwm_action_fn fn,
  zdwm_action_arg_t *arg
) {
  binding_mode_t *mode = binding_table_get_mode(table, mode_id);
  if (!mode) return false;

  modifier_mask_t modifiers = 0;
  xkb_keysym_t keysym       = XKB_KEY_NoSymbol;
  if (!parse_key_sequence(key_sequence, &modifiers, &keysym)) return false;

  key_binding_t *item = array_push(mode->items, mode->count, mode->capacity);
  item->key_str       = key_sequence;
  item->modifiers     = modifiers;
  item->keysym        = keysym;
  item->fn            = fn;
  item->arg           = arg;

  return true;
}

void binding_table_destroy(binding_table_t *table) {
  if (!table) return;

  for (size_t i = 0; i < table->count; ++i) {
    binding_mode_t *mode = &table->modes[i];
    p_delete(&mode->items);
    mode->count    = 0;
    mode->capacity = 0;
  }

  p_delete(&table->modes);
  table->count        = 0;
  table->capacity     = 0;
  table->default_mode = ZDWM_BINDING_MODE_ID_INVALID;

  p_delete(&table);
}

binding_table_t *binding_table_create(void) {
  binding_table_t *table = p_new(binding_table_t, 1);

  /* 默认情况下，默认模式和当前模式都是第一个添加的模式 */
  table->default_mode = 0;
  table->current_mode = 0;

  return table;
}

bool binding_table_set_default_mode(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id
) {
  if (mode_id >= table->count) return false;

  table->default_mode = mode_id;
  return true;
}

bool binding_table_set_current_mode(
  binding_table_t *table,
  zdwm_binding_mode_id_t mode_id
) {
  if (mode_id >= table->count) return false;

  table->current_mode = mode_id;
  return true;
}

bool binding_table_cycle_mode(binding_table_t *table, int delta) {
  if (!table || !table->modes || table->count < 1) return false;

  bool matched = false;
  size_t index = (size_t)table->current_mode;
  if (table->modes[index].id == table->current_mode) {
    matched = true;
  } else {
    for (size_t i = 0; i < table->count; ++i) {
      if (table->modes[i].id == table->current_mode) {
        index   = i;
        matched = true;
        break;
      }
    }
  }

  if (!matched) return false;

  /* 只有一个模式没必要切换 */
  if (table->count == 1) return true;

  int64_t count      = (int64_t)table->count;
  int64_t next_index = ((int64_t)index + (int64_t)delta) % count;
  if (next_index < 0) next_index += count;
  table->current_mode = table->modes[next_index].id;

  return true;
}
