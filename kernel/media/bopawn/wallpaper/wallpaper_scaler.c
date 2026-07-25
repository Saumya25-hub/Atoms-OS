#include "wallpaper_scaler.h"
#include "kernel/core/memory/heap/include/heap.h"
#include <stddef.h>

struct BOSSurface *wallpaper_scaler_scale(struct BOSSurface *src, int target_w,
                                          int target_h, WallpaperScaleMode mode,
                                          uint32_t bg_color) {
  if (!src || !src->framebuffer || src->width <= 0 || src->height <= 0 ||
      target_w <= 0 || target_h <= 0)
    return NULL;
  if ((size_t)target_w > ((size_t)-1) / (size_t)target_h ||
      (size_t)target_w * (size_t)target_h > ((size_t)-1) / sizeof(uint32_t))
    return NULL;

  struct BOSSurface *out = surface_create(target_w, target_h);
  if (!out)
    return NULL;

  // Fill background color
  for (int i = 0; i < target_w * target_h; i++) {
    out->framebuffer[i] = bg_color;
  }

  int src_w = src->width;
  int src_h = src->height;

  if (mode == WALLPAPER_SCALE_STRETCH || mode == WALLPAPER_SCALE_FIT ||
      mode == WALLPAPER_SCALE_FILL) {
    int *x_map = (int *)kmalloc((size_t)target_w * sizeof(int));
    if (x_map) {
      for (int x = 0; x < target_w; x++) {
        int sx = (x * src_w) / target_w;
        if (sx >= src_w)
          sx = src_w - 1;
        x_map[x] = sx;
      }

      for (int y = 0; y < target_h; y++) {
        int sy = (y * src_h) / target_h;
        if (sy >= src_h)
          sy = src_h - 1;

        uint32_t *dest_row = &out->framebuffer[y * target_w];
        uint32_t *src_row = &src->framebuffer[sy * src_w];

        for (int x = 0; x < target_w; x++) {
          dest_row[x] = src_row[x_map[x]];
        }
      }
      kfree(x_map);
    } else {
      surface_destroy(out);
      return NULL;
    }
  } else if (mode == WALLPAPER_SCALE_CENTER) {
    int dx = (target_w - src_w) / 2;
    int dy = (target_h - src_h) / 2;

    for (int y = 0; y < src_h; y++) {
      int dest_y = y + dy;
      if (dest_y < 0 || dest_y >= target_h)
        continue;

      uint32_t *dest_row = &out->framebuffer[dest_y * target_w];
      uint32_t *src_row = &src->framebuffer[y * src_w];

      for (int x = 0; x < src_w; x++) {
        int dest_x = x + dx;
        if (dest_x < 0 || dest_x >= target_w)
          continue;
        dest_row[dest_x] = src_row[x];
      }
    }
  } else if (mode == WALLPAPER_SCALE_TILE) {
    for (int y = 0; y < target_h; y++) {
      int sy = y % src_h;
      uint32_t *dest_row = &out->framebuffer[y * target_w];
      uint32_t *src_row = &src->framebuffer[sy * src_w];
      for (int x = 0; x < target_w; x++) {
        int sx = x % src_w;
        dest_row[x] = src_row[sx];
      }
    }
  }

  return out;
}
