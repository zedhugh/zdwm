#include "image.h"

#include <Imlib2.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

typedef struct image_cache_entry_t {
  char *path;
  cairo_surface_t *surface;
  int width;
  int height;
} image_cache_entry_t;

static image_cache_entry_t *image_cache = nullptr;
static size_t image_cache_count = 0;
static cairo_user_data_key_t image_surface_data_key;

static inline uint32_t premultiply_argb(uint32_t pixel) {
  uint8_t a = (pixel >> 24) & 0xff;
  uint8_t r = (pixel >> 16) & 0xff;
  uint8_t g = (pixel >> 8) & 0xff;
  uint8_t b = pixel & 0xff;
  r = (uint8_t)((r * a + 127) / 255);
  g = (uint8_t)((g * a + 127) / 255);
  b = (uint8_t)((b * a + 127) / 255);
  return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) |
         (uint32_t)b;
}

static bool image_load_surface(const char *path, cairo_surface_t **surface,
                               int *width, int *height) {
  if (!path || !*path || !surface || !width || !height) return false;

  Imlib_Image image = imlib_load_image(path);
  if (!image) return false;

  imlib_context_set_image(image);
  int img_width = imlib_image_get_width();
  int img_height = imlib_image_get_height();
  if (img_width <= 0 || img_height <= 0) {
    imlib_free_image();
    return false;
  }

  DATA32 *src = imlib_image_get_data_for_reading_only();
  if (!src) {
    imlib_free_image();
    return false;
  }

  size_t pixel_count = (size_t)img_width * (size_t)img_height;
  uint32_t *dst = xmalloc((ssize_t)(pixel_count * sizeof(uint32_t)));
  for (size_t i = 0; i < pixel_count; i++) {
    dst[i] = premultiply_argb(src[i]);
  }

  cairo_surface_t *img_surface = cairo_image_surface_create_for_data(
    (unsigned char *)dst, CAIRO_FORMAT_ARGB32, img_width, img_height,
    img_width * (int)sizeof(uint32_t));
  cairo_status_t status = cairo_surface_status(img_surface);
  if (status != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(img_surface);
    p_delete(&dst);
    imlib_free_image();
    return false;
  }

  cairo_surface_set_user_data(img_surface, &image_surface_data_key, dst, free);
  *surface = img_surface;
  *width = img_width;
  *height = img_height;

  imlib_free_image();
  return true;
}

static image_cache_entry_t *image_cache_find(const char *path) {
  if (!path || !*path) return nullptr;
  for (size_t i = 0; i < image_cache_count; i++) {
    image_cache_entry_t *entry = &image_cache[i];
    if (!entry->path) continue;
    if (strcmp(entry->path, path) == 0) return entry;
  }
  return nullptr;
}

static image_cache_entry_t *image_cache_add(const char *path) {
  if (!path || !*path) return nullptr;

  size_t new_count = image_cache_count + 1;
  xrealloc((void **)&image_cache,
           (ssize_t)(new_count * sizeof(image_cache_entry_t)));
  image_cache_entry_t *entry = &image_cache[image_cache_count];
  entry->path = strdup(path);
  entry->surface = nullptr;
  entry->width = 0;
  entry->height = 0;
  image_cache_count = new_count;
  return entry;
}

static image_cache_entry_t *image_cache_get(const char *path) {
  image_cache_entry_t *entry = image_cache_find(path);
  if (entry && entry->surface) return entry;
  if (!entry) entry = image_cache_add(path);
  if (!entry) return nullptr;

  if (entry->surface) {
    cairo_surface_destroy(entry->surface);
    entry->surface = nullptr;
  }

  if (!image_load_surface(path, &entry->surface, &entry->width, &entry->height)) {
    return nullptr;
  }
  return entry;
}

bool image_get_scaled_size(const char *path, int32_t target_height, int *width,
                           int *height) {
  if (!width || !height || target_height <= 0) return false;
  image_cache_entry_t *entry = image_cache_get(path);
  if (!entry || !entry->surface || entry->width <= 0 || entry->height <= 0) {
    return false;
  }

  *height = target_height;
  *width = (entry->width * target_height + entry->height / 2) / entry->height;
  if (*width <= 0) *width = 1;
  return true;
}

bool image_draw(cairo_t *cr, const char *path, int32_t x, int32_t y,
                int32_t width, int32_t height) {
  if (!cr || width <= 0 || height <= 0) return false;
  image_cache_entry_t *entry = image_cache_get(path);
  if (!entry || !entry->surface || entry->width <= 0 || entry->height <= 0) {
    return false;
  }

  cairo_save(cr);
  cairo_translate(cr, (double)x, (double)y);
  cairo_scale(cr, (double)width / (double)entry->width,
              (double)height / (double)entry->height);
  cairo_set_source_surface(cr, entry->surface, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  return true;
}

void image_cache_clean(void) {
  if (!image_cache) return;

  for (size_t i = 0; i < image_cache_count; i++) {
    image_cache_entry_t *entry = &image_cache[i];
    if (entry->surface) {
      cairo_surface_destroy(entry->surface);
      entry->surface = nullptr;
    }
    p_delete(&entry->path);
    entry->width = 0;
    entry->height = 0;
  }

  p_delete(&image_cache);
  image_cache_count = 0;
}
