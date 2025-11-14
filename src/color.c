#include "color.h"

#include "utils.h"

/**
 * @brief 将 16 进制的颜色配置转换成 uint32 格式的 rgba数据
 *
 * @param hex 16 进制的颜色表示，支持的格式有
 * RGB/RGBA/RRGGBB/RRGGBBAA/#RGB/#RGBA/#RRGGBB/#RRGGBBAA
 * @param rgba 返回数据的指针，解析结果会放到这个指针指向的内存中
 * @return 如果 hex 是一个能正常解析的颜色，返回 true ，否则返回 false
 */
static bool hex_color_to_rgba(const char *hex, uint32_t *rgba);
/**
 * @brief 将 rgba 通道顺序的颜色转换成 argb 顺序
 * @description xcb 库用到的颜色需要是 argb 通道顺序
 */
static uint32_t rgba_to_argb(uint32_t rgba);
/**
 * @brief 提取颜色中的各个颜色通道数值
 * @param rgba uint32 表示的颜色
 * @param red red channel, range 0~1
 * @param green green channel, range 0~1
 * @param blue blue channel, range 0~1
 * @param alpha alpha channel, range 0~1
 */
static void extract_color_channel(const uint32_t rgba, double *red,
                                  double *green, double *blue, double *alpha);

void color_parse(const char *hex, color_t *const color) {
  bool success = hex_color_to_rgba(hex, &color->rgba);
  if (!success) fatal("invalid hex color: %s", hex);

  color->argb = rgba_to_argb(color->rgba);
  extract_color_channel(color->rgba, &color->red, &color->green, &color->blue,
                        &color->alpha);
}

bool hex_color_to_rgba(const char *hex, uint32_t *rgba) {
  if (hex[0] == '#') hex++;

  size_t len = strlen(hex);
  char extended[9] = {0};
  uint32_t color = 0x00000000;

  if (len == 3 || len == 4) {
    extended[0] = extended[1] = hex[0];
    extended[2] = extended[3] = hex[1];
    extended[4] = extended[5] = hex[2];
    if (len == 3) {
      extended[6] = extended[7] = 'F';
    } else {
      extended[6] = extended[7] = hex[3];
    }
  } else if (len == 6) {
    strncpy(extended, hex, len);
    extended[6] = extended[7] = 'F';
  } else if (len == 8) {
    strncpy(extended, hex, len);
  } else {
    return false;
  }

  color = strtoul(extended, nullptr, 16) & 0xffffffff;
  *rgba = color;
  return true;
}

#define EXTRACE_COLOR_BIT(C, R) (((C) >> (R)) & 0xff)
uint32_t rgba_to_argb(uint32_t rgba) {
  uint32_t r = EXTRACE_COLOR_BIT(rgba, 24);
  uint32_t g = EXTRACE_COLOR_BIT(rgba, 16);
  uint32_t b = EXTRACE_COLOR_BIT(rgba, 8);
  uint32_t a = EXTRACE_COLOR_BIT(rgba, 0);

  uint32_t argb = (a << 24) | (r << 16) | (g << 8) | b;
  return argb;
}

#define COLOR_SPLIT(C, R) (EXTRACE_COLOR_BIT((C), (R)) / (double)0xff)
void extract_color_channel(const uint32_t rgba, double *red, double *green,
                           double *blue, double *alpha) {
  *red = COLOR_SPLIT(rgba, 24);
  *green = COLOR_SPLIT(rgba, 16);
  *blue = COLOR_SPLIT(rgba, 8);
  *alpha = COLOR_SPLIT(rgba, 0);
}
