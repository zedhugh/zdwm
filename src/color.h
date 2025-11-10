#pragma once

#include <stdint.h>

typedef struct color_t {
  uint32_t rgba; /* use for human reading */
  uint32_t argb; /* use for xcb */

  /* channels of color, use for cairo */
  double red;   /* red channel */
  double green; /* green channel */
  double blue;  /* blue channel */
  double alpha; /* alpha channel */
} color_t;

/**
 * @brief parsing color represented in hexadecimal format
 * @param hex color represented in hexadecimal format, the supported format
   have: RGB/RGBA/RRGGBB/RRGGBBAA/#RGB/#RGBA/#RRGGBB/#RRGGBBAA
 * @param color return the parsed color
 */
void parse_color(const char *hex, color_t *const color);
