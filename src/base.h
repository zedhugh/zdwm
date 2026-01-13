#pragma once

#include <stdint.h>

typedef struct area_t {
  int16_t x, y;
  uint16_t width, height;
} area_t;

typedef struct extent_in_bar_t {
  int16_t start;
  int16_t end;
} extent_in_bar_t;

typedef struct point_t {
  int16_t x, y;
} point_t;
