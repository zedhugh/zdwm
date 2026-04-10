# Zdwm 配置系统设计文档

## 概述

三层配置系统，按优先级从低到高加载：

```
内置默认配置 → XResources → 动态库配置
(后者覆盖前者)
```

---

## 配置层次

### 第一层：内置默认配置
- **位置**: 编译到二进制中
- **作用**: 提供完整的默认配置，确保无外部配置时也能正常运行
- **范围**: 所有配置项

### 第二层：XResources 配置
- **位置**: `~/.Xresources` 或 `~/.config/zdwm/Xresources`
- **作用**: 覆盖外观配置（颜色、字体、边框等）
- **特点**: 无需重新编译，`xrdb` 后重启生效
- **范围**: 仅外观配置

### 第三层：动态库配置
- **位置**: `~/.config/zdwm/config.c` → `config.so`
- **作用**: 高级配置（快捷键、规则、自定义布局等）
- **特点**: 最大灵活性，需编译 C 代码
- **范围**: 所有配置项

---

## 配置加载流程

```
1. 加载内置默认配置
   ↓
2. 尝试加载 XResources
   ├─ 成功 → 覆盖默认配置中的外观项
   └─ 失败 → 跳过，保持默认值
   ↓
3. 尝试加载动态库
   ├─ 存在 → 调用 zdwm_config_init()，增量修改配置
   └─ 不存在 → 跳过
   ↓
4. 应用最终配置
```

---

## 数据结构

### 配置优先级标记

```c
typedef struct config_source_t {
  bool has_xres;       // 是否加载了 XResources
  bool has_user_lib;   // 是否加载了用户动态库
} config_source_t;
```

### 完整配置结构

```c
typedef struct config_t {
  // 外观
  char *font_family;
  uint32_t font_size;
  uint32_t dpi;
  uint16_t border_width;
  uint16_t bar_y_padding;
  uint16_t tag_x_padding;
  color_set_t colors;

  // 快捷键
  const keyboard_t *key_list;
  size_t key_count;

  // 布局
  const layout_t *layout_list;
  size_t layout_count;

  // 标签
  const char *const *tags;

  // 规则
  const rule_t *rules;
  size_t rules_count;

  // 自动启动
  const char *const *autostart_list;

  // 钩子
  void (*hook_manage_new)(client_t *client);
  void (*hook_manage_unmanage)(client_t *client);

  // 元数据
  config_source_t source;
} config_t;
```

---

## API 接口（动态库）

### zdwm_api_t

```c
typedef struct zdwm_api_t {
  uint32_t version;

  // 外观（覆盖 XResources）
  void (*set_font)(const char *family, uint32_t size);
  void (*set_dpi)(uint32_t dpi);
  void (*set_color)(const char *name, const char *hex);
  void (*set_border_width)(uint16_t width);
  void (*set_padding)(uint16_t bar_y, uint16_t tag_x);

  // 快捷键（增量式）
  void (*add_keybinding)(uint32_t modifiers, xcb_keysym_t keysym,
                        void (*action)(const void *),
                        const void *arg);
  void (*remove_keybinding)(uint32_t modifiers, xcb_keysym_t keysym);
  void (*clear_keybindings)(void);

  // 规则
  void (*add_rule)(const char *role, const char *class,
                   int32_t tag_index, bool floating,
                   bool fullscreen, bool maximize,
                   bool switch_to_tag);

  // 自动启动
  void (*add_autostart)(const char *command);

  // 布局
  void (*add_layout)(const char *symbol, void (*arrange)(tag_t *tag));

  // 钩子
  void (*hook_manage_new)(void (*hook)(client_t *client));
  void (*hook_manage_unmanage)(void (*hook)(client_t *client));
} zdwm_api_t;
```

### 用户配置初始化函数

```c
typedef struct zdwm_user_config_t {
  void (*cleanup)(void);
} zdwm_user_config_t;

// 用户在 config.c 中实现此函数（可选）
zdwm_user_config_t *zdwm_config_init(zdwm_api_t *api);
```

---

## XResources 支持的配置项

### 配置项命名规范

```
Zdwm.font.family      # 字体族
Zdwm.font.size        # 字体大小
Zdwm.border.width     # 边框宽度
Zdwm.padding.bar_y    # Bar 上下内边距
Zdwm.padding.tag_x    # Tag 左右内边距
Zdwm.color.bar        # Bar 背景色
Zdwm.color.tag        # Tag 背景色
Zdwm.color.tagActive  # 活动 Tag 背景色
Zdwm.color.tagText    # Tag 文字颜色
Zdwm.color.tagActiveText  # 活动 Tag 文字颜色
Zdwm.color.border     # 边框颜色
Zdwm.color.borderActive  # 活动窗口边框颜色
```

### 数据类型映射

```
XResources 类型    → C 类型
--------------------------→-------------------
字符串             → char *
整数               → uint32_t
颜色 (#RRGGBB)     → color_t (uint32_t argb)
```

---

## 实现要点

### 1. 配置加载函数

```c
// src/config.h
config_t *config_load_full(void);
void config_free(config_t *config);
config_t *config_reload(void);
```

### 2. 加载顺序

```c
// src/config.c
config_t *config_load_full(void) {
  config_t *config = calloc(1, sizeof(config_t));

  // 第一步：默认配置
  config_apply_defaults(config);

  // 第二步：XResources（覆盖外观）
  config_load_xres(config);

  // 第三步：动态库（覆盖所有）
  config_load_user_lib(config);

  return config;
}
```

### 3. XResources 解析

```c
// src/config_xres.c
bool config_load_xres(config_t *config) {
  if (!xres_init_xrm_db()) return false;

  // 覆盖外观配置
  xres_get_string("Zdwm.font.family", &config->font_family, default_font);
  xres_get_uint32("Zdwm.font.size", &config->font_size);
  // ...

  xres_clean();
  return true;
}
```

### 4. 动态库加载

```c
// src/config_lib.c
bool config_load_user_lib(config_t *config) {
  const char *path = "~/.config/zdwm/config.so";

  void *handle = dlopen(path, RTLD_LAZY);
  if (!handle) return false;

  zdwm_user_config_t *(*init)(zdwm_api_t *) = dlsym(handle, "zdwm_config_init");
  if (!init) {
    dlclose(handle);
    return false;  // 没有配置函数是正常的
  }

  zdwm_api_t api = create_api(config);
  init(&api);

  return true;
}
```

---

## 配置热重载

### XResources 热重载

编辑 XResources 后的操作流程：

```bash
# 1. 编辑配置文件
vim ~/.Xresources

# 2. 加载到 X Server
xrdb -merge ~/.Xresources

# 3. 重载 zdwm 配置
killall -HUP zdwm
```

### 实现要点

```c
// src/wm.c

// 重新应用配置
static void wm_reapply_config(void) {
  // 重新初始化字体
  text_reinit_pango_layout(wm.font_family, wm.font_size, wm.dpi);

  // 重新计算 bar 高度
  wm.bar_height = text_get_height() + 2 * wm.padding.bar_y;

  // 刷新所有 monitor 的 bar
  for (monitor_t *m = wm.monitor_list; m; m = m->next) {
    monitor_draw_bar(m);
  }

  xcb_flush(wm.xcb_conn);
}

// SIGHUP 信号处理器
static void reload_config_handler(int sig) {
  log("Received SIGHUP, reloading configuration...");

  // 清理旧的 XResources 数据库
  xres_clean();

  // 重新加载 XResources
  wm_get_xres_config();

  // 重新应用配置
  wm_reapply_config();

  log("Configuration reloaded successfully");
}

// 在 wm_setup_signal() 中注册
void wm_setup_signal(void) {
  // ... 现有信号处理 ...

  signal(SIGHUP, reload_config_handler);
  signal(SIGUSR1, reload_config_handler);  // 备用信号
}
```

### xres 模块需要添加

```c
// src/xres.h
void xres_reload(void);

// src/xres.c
void xres_reload(void) {
  if (wm.xrm) {
    xcb_xrm_database_free(wm.xrm);
    wm.xrm = nullptr;
  }
  xres_init_xrm_db();
}
```

### 完整重载流程

```
用户编辑 XResources
       ↓
xrdb -merge ~/.Xresources  (加载到 X Server)
       ↓
killall -HUP zdwm          (发送重载信号)
       ↓
zdwm 收到 SIGHUP
       ↓
xres_clean()               (清理旧数据库)
       ↓
wm_get_xres_config()       (重新读取 XResources)
       ↓
wm_reapply_config()        (应用新配置)
       ↓
monitor_draw_bar()         (刷新显示)
```

---

## 文件结构

```
src/
├── config.h              # 配置系统头文件
├── config.c              # 配置加载主逻辑
├── config_default.c      # 内置默认配置
├── config_xres.c         # XResources 解析
├── config_lib.c          # 动态库加载
└── config_api.c          # API 实现（供动态库调用）

docs/
└── config_system.md      # 本文档
```

---

## 依赖关系

```
config.c
├── config_default.c  (内置默认值)
├── config_xres.c     (XResources 解析)
│   └── xres.c        (已有)
└── config_lib.c      (动态库加载)
    └── dlopen()
```

---

## 实现优先级

| 优先级 | 模块             | 说明                           |
|--------|------------------|--------------------------------|
| P0     | config_default.c | 定义默认配置值                 |
| P0     | config.c         | 主加载逻辑                     |
| P1     | config_xres.c    | XResources 支持（已有基础）    |
| P1     | 热重载           | SIGHUP 支持（配合 XResources） |
| P1     | config_lib.c     | 动态库加载                     |
| P2     | config_api.c     | API 实现                       |

**P0**: 核心功能，必须实现
**P1**: 重要功能，提升用户体验
**P2**: 增强功能，可后续添加
