#include "../include/bwe.h"
#include "bovisual/Text/font8x16.h"
#include "kernel/ui/bofont/bofont.h"


// Expose clip stack status from bwe_compositor.c
extern bool BWE_GetClip(BWE_Rect *out_rect);

// ============================================================
// Clipped Pixel Plotting Core Helper
// ============================================================
extern void serial_write_direct(const char *str);
static void write_hex(uint32_t val) {
  char buf[16];
  char hex[] = "0123456789ABCDEF";
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 8; i++) {
    buf[9 - i] = hex[val & 0xF];
    val >>= 4;
  }
  buf[10] = '\0';
  serial_write_direct(buf);
}
static void write_int(int val) {
  char buf[16];
  if (val == 0) {
    serial_write_direct("0");
    return;
  }
  if (val < 0) {
    serial_write_direct("-");
    val = -val;
  }
  int i = 0;
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  char rev[16];
  for (int j = 0; j < i; j++) {
    rev[j] = buf[i - j - 1];
  }
  rev[i] = '\0';
  serial_write_direct(rev);
}
#ifndef BWE_ENABLE_AUDIT_LOG
#define BWE_ENABLE_AUDIT_LOG 0
#endif

static int s_log_count = 0;
void audit_log_draw(const char *func, int x, int y, int w, int h, int r,
                    uint32_t color) {
#if BWE_ENABLE_AUDIT_LOG
  if ((y - r) >= 100 && y >= 100)
    return;
  if (s_log_count > 500)
    return;
  s_log_count++;
  serial_write_direct("[AUDIT] ");
  serial_write_direct(func);
  serial_write_direct(" X=");
  write_int(x);
  serial_write_direct(" Y=");
  write_int(y);
  serial_write_direct(" W=");
  write_int(w);
  serial_write_direct(" H=");
  write_int(h);
  serial_write_direct(" R=");
  write_int(r);
  serial_write_direct(" C=");
  write_hex(color);
  serial_write_direct("\n");
#else
  (void)func; (void)x; (void)y; (void)w; (void)h; (void)r; (void)color;
#endif
}

static inline void plot_pixel(const BVFramebuffer *fb, int32_t x, int32_t y,
                              uint32_t color, const BWE_Rect *clip) {
  if (x >= clip->x && x < clip->x + clip->width && y >= clip->y &&
      y < clip->y + clip->height) {
    if (x >= 0 && x < (int32_t)fb->width && y >= 0 && y < (int32_t)fb->height) {
      fb->buffer[y * (fb->pitch / 4) + x] = color;
    }
  }
}

// ============================================================
// Paint Engine Drawing Primitives
// ============================================================

void BWE_FillRect(const BVFramebuffer *fb, int32_t x, int32_t y, int32_t w,
                  int32_t h, uint32_t color) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);
  audit_log_draw("BWE_FillRect", x, y, w, h, 0, color);
  if (!fb || w <= 0 || h <= 0)
    return;

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t x1 = clip.x + x;
  int32_t y1 = clip.y + y;
  int32_t x2 = x1 + w;
  int32_t y2 = y1 + h;

  // Bounds clipping intersection check
  if (x1 < clip.x)
    x1 = clip.x;
  if (y1 < clip.y)
    y1 = clip.y;
  if (x2 > clip.x + clip.width)
    x2 = clip.x + clip.width;
  if (y2 > clip.y + clip.height)
    y2 = clip.y + clip.height;

  if (x1 >= x2 || y1 >= y2)
    return;

  uint32_t pitch_w = fb->pitch / 4;
  extern void heap_check_external_write(uint64_t dst_addr, size_t len,
                                        const char *caller, uint64_t rip);
  if (fb->buffer) {
    uint64_t start_addr = (uint64_t)(&fb->buffer[y1 * pitch_w + x1]);
    uint64_t end_addr = (uint64_t)(&fb->buffer[(y2 - 1) * pitch_w + x2]);
    if (end_addr > start_addr) {
      heap_check_external_write(start_addr, end_addr - start_addr,
                                "BWE_FillRect",
                                (uint64_t)__builtin_return_address(0));
    }
  }
  for (int32_t cy = y1; cy < y2; cy++) {
    uint32_t offset = cy * pitch_w;
    for (int32_t cx = x1; cx < x2; cx++) {
      fb->buffer[offset + cx] = color;
    }
  }
}

void BWE_DrawRect(const BVFramebuffer *fb, int32_t x, int32_t y, int32_t w,
                  int32_t h, uint32_t color, uint32_t thickness) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);
  audit_log_draw("BWE_DrawRect", x, y, w, h, thickness, color);
  if (!fb || w <= 0 || h <= 0 || thickness == 0)
    return;

  // Top edge
  BWE_FillRect(fb, x, y, w, (int32_t)thickness, color);
  // Bottom edge
  BWE_FillRect(fb, x, y + h - (int32_t)thickness, w, (int32_t)thickness, color);
  // Left edge
  BWE_FillRect(fb, x, y, (int32_t)thickness, h, color);
  // Right edge
  BWE_FillRect(fb, x + w - (int32_t)thickness, y, (int32_t)thickness, h, color);
}

void BWE_DrawLine(const BVFramebuffer *fb, int32_t x1, int32_t y1, int32_t x2,
                  int32_t y2, uint32_t color) {
  if (!fb)
    return;

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t dx = (x2 - x1 >= 0) ? (x2 - x1) : (x1 - x2);
  int32_t dy = (y2 - y1 >= 0) ? (y2 - y1) : (y1 - y2);
  int32_t sx = (x1 < x2) ? 1 : -1;
  int32_t sy = (y1 < y2) ? 1 : -1;
  int32_t err = dx - dy;

  int32_t cx = x1;
  int32_t cy = y1;

  while (1) {
    plot_pixel(fb, clip.x + cx, clip.y + cy, color, &clip);
    if (cx == x2 && cy == y2)
      break;
    int32_t e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      cx += sx;
    }
    if (e2 < dx) {
      err += dx;
      cy += sy;
    }
  }
}

void BWE_DrawText(const BVFramebuffer *fb, const char *text, int32_t x,
                  int32_t y, uint32_t color, BWE_Font *font) {
  (void)font;
  if (!text || text[0] == '\0')
    return;
  BOFont_DrawTextRoleTarget(fb, BOFONT_ROLE_UI_REGULAR, text, x, y, color);
}

void BWE_DrawTextRole(const BVFramebuffer *fb, const char *text, int32_t x,
                      int32_t y, uint32_t color, uint32_t role) {
  if (!text || text[0] == '\0')
    return;
  BOFont_DrawTextRoleTarget(fb, (BOFontRole)role, text, x, y, color);
}

void BWE_DrawBitmap(const BVFramebuffer *fb, const uint32_t *pixels,
                    int32_t dest_x, int32_t dest_y, int32_t dest_w,
                    int32_t dest_h, int32_t src_x, int32_t src_y, int32_t src_w,
                    int32_t src_h, int32_t bmp_pitch) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);
  audit_log_draw("BWE_DrawBitmap", dest_x, dest_y, dest_w, dest_h, 0, 0);
  if (!fb || !pixels || dest_w <= 0 || dest_h <= 0 || src_w <= 0 || src_h <= 0)
    return;

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  uint32_t pitch_words = (uint32_t)bmp_pitch / 4;

  for (int32_t dy = 0; dy < dest_h; dy++) {
    int32_t py = clip.y + dest_y + dy;
    if (py < clip.y || py >= clip.y + clip.height) continue;

    int64_t sy_fp = (dest_h > 1) ? (((int64_t)(src_y + dy) * src_h) << 16) / dest_h : 0;
    int32_t sy0 = (int32_t)(sy_fp >> 16);
    if (sy0 < 0) sy0 = 0;
    if (sy0 >= src_h) sy0 = src_h - 1;
    int32_t sy1 = (sy0 + 1 < src_h) ? sy0 + 1 : sy0;
    uint32_t fy = (uint32_t)((sy_fp & 0xFFFF) >> 8); // 0..256

    for (int32_t dx = 0; dx < dest_w; dx++) {
      int32_t px = clip.x + dest_x + dx;
      if (px < clip.x || px >= clip.x + clip.width) continue;

      int64_t sx_fp = (dest_w > 1) ? (((int64_t)(src_x + dx) * src_w) << 16) / dest_w : 0;
      int32_t sx0 = (int32_t)(sx_fp >> 16);
      if (sx0 < 0) sx0 = 0;
      if (sx0 >= src_w) sx0 = src_w - 1;
      int32_t sx1 = (sx0 + 1 < src_w) ? sx0 + 1 : sx0;
      uint32_t fx = (uint32_t)((sx_fp & 0xFFFF) >> 8); // 0..256

      if (dest_w == src_w && dest_h == src_h) {
        uint32_t color = pixels[sy0 * pitch_words + sx0];
        if ((color >> 24) != 0) {
          plot_pixel(fb, px, py, color, &clip);
        }
      } else {
        uint32_t c00 = pixels[sy0 * pitch_words + sx0];
        uint32_t c01 = pixels[sy0 * pitch_words + sx1];
        uint32_t c10 = pixels[sy1 * pitch_words + sx0];
        uint32_t c11 = pixels[sy1 * pitch_words + sx1];

        uint32_t w00 = (256 - fx) * (256 - fy);
        uint32_t w01 = fx * (256 - fy);
        uint32_t w10 = (256 - fx) * fy;
        uint32_t w11 = fx * fy;

        uint32_t r = (((c00 >> 16) & 0xFF) * w00 + ((c01 >> 16) & 0xFF) * w01 + ((c10 >> 16) & 0xFF) * w10 + ((c11 >> 16) & 0xFF) * w11) >> 16;
        uint32_t g = (((c00 >> 8) & 0xFF) * w00 + ((c01 >> 8) & 0xFF) * w01 + ((c10 >> 8) & 0xFF) * w10 + ((c11 >> 8) & 0xFF) * w11) >> 16;
        uint32_t b = ((c00 & 0xFF) * w00 + (c01 & 0xFF) * w01 + (c10 & 0xFF) * w10 + (c11 & 0xFF) * w11) >> 16;

        uint32_t color = 0xFF000000U | (r << 16) | (g << 8) | b;
        plot_pixel(fb, px, py, color, &clip);
      }
    }
  }
}

// Corner masking helper for R = 6px outer rounded window corners
static inline bool is_outside_corner(int32_t x, int32_t y,
                                     const BWE_Rect *bounds, int32_t r) {
  int32_t left_cx = bounds->x + r;
  int32_t right_cx = bounds->x + bounds->width - r - 1;
  int32_t top_cy = bounds->y + r;
  int32_t bottom_cy = bounds->y + bounds->height - r - 1;

  if (x < left_cx && y < top_cy) {
    int32_t dx = x - left_cx;
    int32_t dy = y - top_cy;
    return (dx * dx + dy * dy) > (r * r);
  }
  if (x > right_cx && y < top_cy) {
    int32_t dx = x - right_cx;
    int32_t dy = y - top_cy;
    return (dx * dx + dy * dy) > (r * r);
  }
  if (x < left_cx && y > bottom_cy) {
    int32_t dx = x - left_cx;
    int32_t dy = y - bottom_cy;
    return (dx * dx + dy * dy) > (r * r);
  }
  if (x > right_cx && y > bottom_cy) {
    int32_t dx = x - right_cx;
    int32_t dy = y - bottom_cy;
    return (dx * dx + dy * dy) > (r * r);
  }
  return false;
}

// Authoritative 1px outer hairline boundary detector
static inline bool is_border_pixel(int32_t x, int32_t y, const BWE_Rect *bounds,
                                   int32_t r) {
  if (is_outside_corner(x, y, bounds, r))
    return false;

  int32_t L = bounds->x;
  int32_t T = bounds->y;
  int32_t R = bounds->x + bounds->width - 1;
  int32_t B = bounds->y + bounds->height - 1;

  if (x == L || x == R || y == T || y == B)
    return true;

  if (is_outside_corner(x - 1, y, bounds, r) ||
      is_outside_corner(x + 1, y, bounds, r) ||
      is_outside_corner(x, y - 1, bounds, r) ||
      is_outside_corner(x, y + 1, bounds, r)) {
    return true;
  }

  return false;
}

void BWE_FillRectEx(const BVFramebuffer *fb, int32_t x, int32_t y, int32_t w,
                   int32_t h, uint32_t color_start, uint32_t color_end,
                   uint8_t gradient_mode, int32_t corner_radius) {
  if (!fb || w <= 0 || h <= 0)
    return;

  // Fast path for simple solid rectangular fill
  if (gradient_mode == 0 && corner_radius <= 0 && color_start == color_end) {
    BWE_FillRect(fb, x, y, w, h, color_start);
    return;
  }

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t x1 = x;
  int32_t y1 = y;
  int32_t x2 = x + w;
  int32_t y2 = y + h;

  if (x1 < clip.x) x1 = clip.x;
  if (y1 < clip.y) y1 = clip.y;
  if (x2 > clip.x + clip.width) x2 = clip.x + clip.width;
  if (y2 > clip.y + clip.height) y2 = clip.y + clip.height;

  if (x1 >= x2 || y1 >= y2)
    return;

  uint32_t pitch_w = fb->pitch / 4;
  BWE_Rect bounds = { .x = x, .y = y, .width = w, .height = h };

  int32_t r1 = (color_start >> 16) & 0xFF;
  int32_t g1 = (color_start >> 8) & 0xFF;
  int32_t b1 = color_start & 0xFF;
  int32_t a1 = (color_start >> 24) & 0xFF;

  int32_t r2 = (color_end >> 16) & 0xFF;
  int32_t g2 = (color_end >> 8) & 0xFF;
  int32_t b2 = color_end & 0xFF;
  int32_t a2 = (color_end >> 24) & 0xFF;

  for (int32_t cy = y1; cy < y2; cy++) {
    uint32_t offset = cy * pitch_w;

    uint32_t row_color = color_start;
    if (gradient_mode == 1) { // VERTICAL
      int32_t step = cy - y;
      int32_t total = h > 1 ? h - 1 : 1;
      int32_t cr = r1 + ((r2 - r1) * step) / total;
      int32_t cg = g1 + ((g2 - g1) * step) / total;
      int32_t cb = b1 + ((b2 - b1) * step) / total;
      int32_t ca = a1 + ((a2 - a1) * step) / total;
      if (cr < 0) cr = 0; else if (cr > 255) cr = 255;
      if (cg < 0) cg = 0; else if (cg > 255) cg = 255;
      if (cb < 0) cb = 0; else if (cb > 255) cb = 255;
      if (ca < 0) ca = 0; else if (ca > 255) ca = 255;
      row_color = ((uint32_t)ca << 24) | ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;
    }

    for (int32_t cx = x1; cx < x2; cx++) {
      if (corner_radius > 0 && is_outside_corner(cx, cy, &bounds, corner_radius)) {
        continue;
      }

      uint32_t final_color = row_color;
      if (gradient_mode == 2) { // HORIZONTAL
        int32_t step = cx - x;
        int32_t total = w > 1 ? w - 1 : 1;
        int32_t cr = r1 + ((r2 - r1) * step) / total;
        int32_t cg = g1 + ((g2 - g1) * step) / total;
        int32_t cb = b1 + ((b2 - b1) * step) / total;
        int32_t ca = a1 + ((a2 - a1) * step) / total;
        if (cr < 0) cr = 0; else if (cr > 255) cr = 255;
        if (cg < 0) cg = 0; else if (cg > 255) cg = 255;
        if (cb < 0) cb = 0; else if (cb > 255) cb = 255;
        if (ca < 0) ca = 0; else if (ca > 255) ca = 255;
        final_color = ((uint32_t)ca << 24) | ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;
      }

      fb->buffer[offset + cx] = final_color;
    }
  }
}

void BWE_DrawRectEx(const BVFramebuffer *fb, int32_t x, int32_t y, int32_t w,
                   int32_t h, uint32_t color, uint32_t thickness, int32_t corner_radius) {
  if (!fb || w <= 0 || h <= 0 || thickness == 0)
    return;

  if (corner_radius <= 0) {
    BWE_DrawRect(fb, x, y, w, h, color, thickness);
    return;
  }

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  BWE_Rect bounds = { .x = x, .y = y, .width = w, .height = h };
  uint32_t pitch_w = fb->pitch / 4;

  int32_t x1 = x;
  int32_t y1 = y;
  int32_t x2 = x + w;
  int32_t y2 = y + h;

  if (x1 < clip.x) x1 = clip.x;
  if (y1 < clip.y) y1 = clip.y;
  if (x2 > clip.x + clip.width) x2 = clip.x + clip.width;
  if (y2 > clip.y + clip.height) y2 = clip.y + clip.height;

  for (int32_t cy = y1; cy < y2; cy++) {
    uint32_t offset = cy * pitch_w;
    for (int32_t cx = x1; cx < x2; cx++) {
      if (is_border_pixel(cx, cy, &bounds, corner_radius)) {
        fb->buffer[offset + cx] = color;
      }
    }
  }
}

#include "kernel/wm/botheme/botheme.h"

void BWE_DrawBorder(const BVFramebuffer *fb, const BWE_Rect *bounds,
                    uint32_t color, bool active) {
  (void)color;
  if (!fb || bounds->width <= 0 || bounds->height <= 0)
    return;

  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t r = 6;
  uint32_t outer_line_color =
      active ? BOTHEME_GetColor(BOTHEME_WINDOW_BORDER_ACTIVE)
             : BOTHEME_GetColor(BOTHEME_WINDOW_BORDER_INACTIVE);
  uint32_t frame_bg_color = active
                                ? BOTHEME_GetColor(BOTHEME_FRAME_BG_ACTIVE)
                                : BOTHEME_GetColor(BOTHEME_FRAME_BG_INACTIVE);

  int32_t L = bounds->x;
  int32_t T = bounds->y;
  int32_t R = bounds->x + bounds->width - 1;
  int32_t B = bounds->y + bounds->height - 1;

  // Unified pass: 1px continuous outer hairline & inner side/bottom frame
  // margins
  for (int32_t y = T; y <= B; y++) {
    for (int32_t x = L; x <= R; x++) {
      if (is_outside_corner(x, y, bounds, r))
        continue;

      if (is_border_pixel(x, y, bounds, r)) {
        plot_pixel(fb, x, y, outer_line_color, &clip);
      } else {
        // Inner frame margin fill (below 35px titlebar region):
        // Left margin: x in [L+1 .. L+4], y in [T+35 .. B-1]
        // Right margin: x in [R-4 .. R-1], y in [T+35 .. B-1]
        // Bottom margin: y in [B-4 .. B-1], x in [L+5 .. R-5]
        if (y >= T + 35 && y <= B - 1) {
          if (x < L + 5 || x > R - 5 || y > B - 5) {
            plot_pixel(fb, x, y, frame_bg_color, &clip);
          }
        }
      }
    }
  }
}

void BWE_DrawShadow(const BVFramebuffer *fb, const BWE_Rect *bounds,
                    bool active) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);
  audit_log_draw("BWE_DrawShadow", bounds->x, bounds->y, bounds->width,
                 bounds->height, active, 0);
  // Multi-layered soft drop shadow starting strictly outside the physical
  // window frame
  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t shadow_dist = active ? 8 : 5;
  uint32_t base_shadow_color = BOTHEME_GetColor(BOTHEME_SHADOW_COLOR);
  uint32_t base_alpha = (base_shadow_color >> 24) & 0xFF;
  if (base_alpha == 0)
    base_alpha = 0x2A;

  // Right shadow (starts at x = R + 1)
  for (int32_t s = 0; s < shadow_dist; s++) {
    int32_t x = bounds->x + bounds->width + s;
    uint32_t alpha = (base_alpha * (shadow_dist - s)) / shadow_dist;
    uint32_t shadow_color = (alpha << 24);
    for (int32_t y = bounds->y + 6; y < bounds->y + bounds->height + s; y++) {
      plot_pixel(fb, x, y, shadow_color, &clip);
    }
  }

  // Bottom shadow (starts at y = B + 1)
  for (int32_t s = 0; s < shadow_dist; s++) {
    int32_t y = bounds->y + bounds->height + s;
    uint32_t alpha = (base_alpha * (shadow_dist - s)) / shadow_dist;
    uint32_t shadow_color = (alpha << 24);
    for (int32_t x = bounds->x + 6; x < bounds->x + bounds->width + shadow_dist;
         x++) {
      plot_pixel(fb, x, y, shadow_color, &clip);
    }
  }
}

static inline void draw_circle_badge(const BVFramebuffer *fb, int32_t cx,
                                     int32_t cy, int32_t cr,
                                     uint32_t fill_color, uint32_t border_color,
                                     const BWE_Rect *clip) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);
  audit_log_draw("draw_circle_badge", cx, cy, 0, 0, cr, fill_color);
  for (int32_t dy = -cr; dy <= cr; dy++) {
    for (int32_t dx = -cr; dx <= cr; dx++) {
      if (dx * dx + dy * dy <= cr * cr) {
        uint32_t col = (dx * dx + dy * dy >= (cr - 1) * (cr - 1)) ? border_color
                                                                  : fill_color;
        plot_pixel(fb, cx + dx, cy + dy, col, clip);
      }
    }
  }
}

void BWE_DrawTitleBar(const BVFramebuffer *fb, const BWE_Rect *bounds,
                      const char *title, bool active, bool resizable) {
  BWE_Rect clip;
  if (!BWE_GetClip(&clip)) {
    clip.x = 0;
    clip.y = 0;
    clip.width = (int32_t)fb->width;
    clip.height = (int32_t)fb->height;
  }

  int32_t L = bounds->x;
  int32_t T = bounds->y;
  int32_t r = 6;

  int32_t tx = L + 1;
  int32_t ty = T + 1;
  int32_t tw = bounds->width - 2;
  int32_t th = 34;

  // BOTHEME gradient palette lookups
  uint32_t color_top = active ? BOTHEME_GetColor(BOTHEME_TITLE_ACTIVE_TOP)
                              : BOTHEME_GetColor(BOTHEME_TITLE_INACTIVE_TOP);
  uint32_t color_bottom = active
                              ? BOTHEME_GetColor(BOTHEME_TITLE_ACTIVE_BOTTOM)
                              : BOTHEME_GetColor(BOTHEME_TITLE_INACTIVE_BOTTOM);

  // 1. Draw Titlebar Gradient (rows y = 0 to 32, corresponding to py = T+1 to
  // T+33)
  int32_t r1 = (color_top >> 16) & 0xFF;
  int32_t g1 = (color_top >> 8) & 0xFF;
  int32_t b1 = color_top & 0xFF;

  int32_t r2 = (color_bottom >> 16) & 0xFF;
  int32_t g2 = (color_bottom >> 8) & 0xFF;
  int32_t b2 = color_bottom & 0xFF;

  for (int32_t y = 0; y < th - 1; y++) {
    int32_t cr = r1 + ((r2 - r1) * y) / (th - 1);
    int32_t cg = g1 + ((g2 - g1) * y) / (th - 1);
    int32_t cb = b1 + ((b2 - b1) * y) / (th - 1);

    if (cr < 0)
      cr = 0;
    else if (cr > 255)
      cr = 255;
    if (cg < 0)
      cg = 0;
    else if (cg > 255)
      cg = 255;
    if (cb < 0)
      cb = 0;
    else if (cb > 255)
      cb = 255;

    uint32_t line_color =
        0xFF000000 | ((uint32_t)cr << 16) | ((uint32_t)cg << 8) | (uint32_t)cb;
    int32_t py = ty + y;

    for (int32_t x = tx; x < tx + tw; x++) {
      if (!is_outside_corner(x, py, bounds, r) &&
          !is_border_pixel(x, py, bounds, r)) {
        plot_pixel(fb, x, py, line_color, &clip);
      }
    }
  }

  // 2. 1px Titlebar Bottom Accent line at py = T + 34
  uint32_t accent_color =
      active ? BOTHEME_GetColor(BOTHEME_ACCENT_LINE)
             : BOTHEME_GetColor(BOTHEME_WINDOW_BORDER_INACTIVE);
  int32_t py_accent = ty + th - 1;
  for (int32_t x = tx; x < tx + tw; x++) {
    if (!is_outside_corner(x, py_accent, bounds, r) &&
        !is_border_pixel(x, py_accent, bounds, r)) {
      plot_pixel(fb, x, py_accent, accent_color, &clip);
    }
  }

  // 3. Render Title Text
  if (title && title[0] != '\0') {
    uint32_t text_color = active
                              ? BOTHEME_GetColor(BOTHEME_TITLE_TEXT_ACTIVE)
                              : BOTHEME_GetColor(BOTHEME_TITLE_TEXT_INACTIVE);
    BWE_DrawTextRole(fb, title, tx + 11, ty + 8, text_color,
                     BOFONT_ROLE_UI_BOLD);
  }

  // 4. Redesigned Integrated Control Badges (14px diameter circular badges)
  int32_t btn_radius = 7;
  int32_t btn_cy = ty + 16;

  // Close Button (Red Circle with 'x')
  int32_t close_cx = tx + tw - 16;
  draw_circle_badge(fb, close_cx, btn_cy, btn_radius, 0xFFEF4444, 0xFFDC2626,
                    &clip);
  plot_pixel(fb, close_cx - 2, btn_cy - 2, 0xFFFFFFFF, &clip);
  plot_pixel(fb, close_cx + 2, btn_cy - 2, 0xFFFFFFFF, &clip);
  plot_pixel(fb, close_cx, btn_cy, 0xFFFFFFFF, &clip);
  plot_pixel(fb, close_cx - 2, btn_cy + 2, 0xFFFFFFFF, &clip);
  plot_pixel(fb, close_cx + 2, btn_cy + 2, 0xFFFFFFFF, &clip);

  // Maximize Button (Amber Circle with box frame)
  if (resizable) {
    int32_t max_cx = tx + tw - 38;
    draw_circle_badge(fb, max_cx, btn_cy, btn_radius, 0xFFF59E0B, 0xFFD97706,
                      &clip);
    for (int32_t bx = max_cx - 2; bx <= max_cx + 2; bx++) {
      plot_pixel(fb, bx, btn_cy - 2, 0xFFFFFFFF, &clip);
      plot_pixel(fb, bx, btn_cy + 2, 0xFFFFFFFF, &clip);
    }
    plot_pixel(fb, max_cx - 2, btn_cy - 1, 0xFFFFFFFF, &clip);
    plot_pixel(fb, max_cx - 2, btn_cy, 0xFFFFFFFF, &clip);
    plot_pixel(fb, max_cx - 2, btn_cy + 1, 0xFFFFFFFF, &clip);
    plot_pixel(fb, max_cx + 2, btn_cy - 1, 0xFFFFFFFF, &clip);
    plot_pixel(fb, max_cx + 2, btn_cy, 0xFFFFFFFF, &clip);
    plot_pixel(fb, max_cx + 2, btn_cy + 1, 0xFFFFFFFF, &clip);
  }

  // Minimize Button (Green Circle with '-')
  int32_t min_cx = tx + tw - (resizable ? 60 : 38);
  draw_circle_badge(fb, min_cx, btn_cy, btn_radius, 0xFF10B981, 0xFF059669,
                    &clip);
  for (int32_t bx = min_cx - 3; bx <= min_cx + 3; bx++) {
    plot_pixel(fb, bx, btn_cy, 0xFFFFFFFF, &clip);
  }
}
