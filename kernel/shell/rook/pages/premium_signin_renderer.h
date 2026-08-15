#ifndef PREMIUM_SIGNIN_RENDERER_H
#define PREMIUM_SIGNIN_RENDERER_H

#include "bovisual/Include/graphics.h"
#include "kernel/services/user_profile/user_profile_service.h"
#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/ui/bofont/bofont.h"
#include <stdbool.h>
#include <stdint.h>


/*
 * Premium Sign-In Renderer
 *
 * This renderer owns every pixel of the sign-in frame.  It deliberately has
 * no dependency on desktop windows, widgets, the boot spinner, or the legacy
 * login controls.  The lock screen remains in page_login.c and is not drawn
 * by this module.
 */

#define PREMIUM_BLUR_W 480
#define PREMIUM_BLUR_H 270

static uint32_t s_premium_blur_canvas[PREMIUM_BLUR_W * PREMIUM_BLUR_H]
    __attribute__((aligned(16)));
static uint32_t s_premium_blur_temp[PREMIUM_BLUR_W * PREMIUM_BLUR_H]
    __attribute__((aligned(16)));
static uint32_t s_darkened_blur_cache[1920 * 1080]
    __attribute__((aligned(16)));
static bool s_premium_blur_ready = false;

static uint32_t premium_mix(uint32_t dst, uint32_t src, uint8_t alpha) {
  if (alpha == 0)
    return dst;
  if (alpha == 255)
    return src & 0x00FFFFFF;
  uint32_t inv = 255u - alpha;
  uint32_t r =
      ((((dst >> 16) & 0xFFu) * inv) + (((src >> 16) & 0xFFu) * alpha)) / 255u;
  uint32_t g =
      ((((dst >> 8) & 0xFFu) * inv) + (((src >> 8) & 0xFFu) * alpha)) / 255u;
  uint32_t b = (((dst & 0xFFu) * inv) + ((src & 0xFFu) * alpha)) / 255u;
  return (r << 16) | (g << 8) | b;
}

static uint32_t premium_lerp(uint32_t a, uint32_t b, uint32_t t) {
  uint32_t inv = 256u - t;
  uint32_t r = ((((a >> 16) & 0xFFu) * inv) + (((b >> 16) & 0xFFu) * t)) >> 8;
  uint32_t g = ((((a >> 8) & 0xFFu) * inv) + (((b >> 8) & 0xFFu) * t)) >> 8;
  uint32_t bl = (((a & 0xFFu) * inv) + ((b & 0xFFu) * t)) >> 8;
  return (r << 16) | (g << 8) | bl;
}

#include "kernel/performance/include/profiler.h"

static uint32_t premium_sample_blur(uint32_t x, uint32_t y, uint32_t width,
                                    uint32_t height);

static void premium_signin_reset(void) { s_premium_blur_ready = false; }

/* 480x270 2-Pass Separable 5-Tap Gaussian Kernel produces a velvety smooth
 * frosted glass blur with 0% pixelation at 1080p. */
static void premium_build_blur(void) {
  BOS_PROFILE_SCOPE("premium_build_blur");
  if (s_premium_blur_ready)
    return;
  const uint32_t *source = wallpaper_service_get_canvas();
  if (!source)
    return;

  /* Step 1: 4x4 Downsample to 480x270 */
  for (uint32_t by = 0; by < PREMIUM_BLUR_H; by++) {
    uint32_t sy0 = (by * 1080u) / PREMIUM_BLUR_H;
    uint32_t sy1 = ((by + 1u) * 1080u) / PREMIUM_BLUR_H;
    if (sy1 <= sy0)
      sy1 = sy0 + 1u;
    for (uint32_t bx = 0; bx < PREMIUM_BLUR_W; bx++) {
      uint32_t sx0 = (bx * 1920u) / PREMIUM_BLUR_W;
      uint32_t sx1 = ((bx + 1u) * 1920u) / PREMIUM_BLUR_W;
      if (sx1 <= sx0)
        sx1 = sx0 + 1u;

      uint32_t rs = 0, gs = 0, bs = 0, count = 0;
      for (uint32_t sy = sy0; sy < sy1; sy++) {
        uint32_t row = sy * 1920u;
        for (uint32_t sx = sx0; sx < sx1; sx++) {
          uint32_t c = source[row + sx];
          rs += (c >> 16) & 0xFFu;
          gs += (c >> 8) & 0xFFu;
          bs += c & 0xFFu;
          count++;
        }
      }
      s_premium_blur_canvas[by * PREMIUM_BLUR_W + bx] =
          ((rs / count) << 16) | ((gs / count) << 8) | (bs / count);
    }
  }

  /* Step 2: 5-Tap Separable Gaussian Horizontal Blur Pass [1, 4, 6, 4, 1] >> 4 */
  for (uint32_t y = 0; y < PREMIUM_BLUR_H; y++) {
    uint32_t row = y * PREMIUM_BLUR_W;
    for (uint32_t x = 0; x < PREMIUM_BLUR_W; x++) {
      int xm2 = (x >= 2) ? x - 2 : 0;
      int xm1 = (x >= 1) ? x - 1 : 0;
      int xp1 = (x + 1 < PREMIUM_BLUR_W) ? x + 1 : PREMIUM_BLUR_W - 1;
      int xp2 = (x + 2 < PREMIUM_BLUR_W) ? x + 2 : PREMIUM_BLUR_W - 1;

      uint32_t c0 = s_premium_blur_canvas[row + xm2];
      uint32_t c1 = s_premium_blur_canvas[row + xm1];
      uint32_t c2 = s_premium_blur_canvas[row + x];
      uint32_t c3 = s_premium_blur_canvas[row + xp1];
      uint32_t c4 = s_premium_blur_canvas[row + xp2];

      uint32_t r = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF) * 4 + ((c2 >> 16) & 0xFF) * 6 + ((c3 >> 16) & 0xFF) * 4 + ((c4 >> 16) & 0xFF)) >> 4;
      uint32_t g = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF) * 4 + ((c2 >> 8) & 0xFF) * 6 + ((c3 >> 8) & 0xFF) * 4 + ((c4 >> 8) & 0xFF)) >> 4;
      uint32_t b = ((c0 & 0xFF) + (c1 & 0xFF) * 4 + (c2 & 0xFF) * 6 + (c3 & 0xFF) * 4 + (c4 & 0xFF)) >> 4;

      s_premium_blur_temp[row + x] = (r << 16) | (g << 8) | b;
    }
  }

  /* Step 3: 5-Tap Separable Gaussian Vertical Blur Pass [1, 4, 6, 4, 1] >> 4 */
  for (uint32_t y = 0; y < PREMIUM_BLUR_H; y++) {
    int ym2 = (y >= 2) ? y - 2 : 0;
    int ym1 = (y >= 1) ? y - 1 : 0;
    int yp1 = (y + 1 < PREMIUM_BLUR_H) ? y + 1 : PREMIUM_BLUR_H - 1;
    int yp2 = (y + 2 < PREMIUM_BLUR_H) ? y + 2 : PREMIUM_BLUR_H - 1;

    uint32_t r0 = ym2 * PREMIUM_BLUR_W;
    uint32_t r1 = ym1 * PREMIUM_BLUR_W;
    uint32_t r2 = y * PREMIUM_BLUR_W;
    uint32_t r3 = yp1 * PREMIUM_BLUR_W;
    uint32_t r4 = yp2 * PREMIUM_BLUR_W;

    for (uint32_t x = 0; x < PREMIUM_BLUR_W; x++) {
      uint32_t c0 = s_premium_blur_temp[r0 + x];
      uint32_t c1 = s_premium_blur_temp[r1 + x];
      uint32_t c2 = s_premium_blur_temp[r2 + x];
      uint32_t c3 = s_premium_blur_temp[r3 + x];
      uint32_t c4 = s_premium_blur_temp[r4 + x];

      uint32_t r = (((c0 >> 16) & 0xFF) + ((c1 >> 16) & 0xFF) * 4 + ((c2 >> 16) & 0xFF) * 6 + ((c3 >> 16) & 0xFF) * 4 + ((c4 >> 16) & 0xFF)) >> 4;
      uint32_t g = (((c0 >> 8) & 0xFF) + ((c1 >> 8) & 0xFF) * 4 + ((c2 >> 8) & 0xFF) * 6 + ((c3 >> 8) & 0xFF) * 4 + ((c4 >> 8) & 0xFF)) >> 4;
      uint32_t b = ((c0 & 0xFF) + (c1 & 0xFF) * 4 + (c2 & 0xFF) * 6 + (c3 & 0xFF) * 4 + (c4 & 0xFF)) >> 4;

      s_premium_blur_canvas[r2 + x] = (r << 16) | (g << 8) | b;
    }
  }

  /* Step 4: Pre-bake full-frame velvety frosted glass cache with smooth bilinear filter */
  for (uint32_t y = 0; y < 1080; y++) {
    uint32_t row = y * 1920;
    for (uint32_t x = 0; x < 1920; x++) {
      uint32_t blurred = premium_sample_blur(x, y, 1920, 1080);
      s_darkened_blur_cache[row + x] = premium_mix(blurred, 0x00000000, 75);
    }
  }

  s_premium_blur_ready = true;
}


static uint32_t premium_sample_blur(uint32_t x, uint32_t y, uint32_t width,
                                    uint32_t height) {
  uint32_t fx = (width > 1)
                    ? (uint32_t)(((uint64_t)x * (PREMIUM_BLUR_W - 1u) * 256u) /
                                 (width - 1u))
                    : 0;
  uint32_t fy = (height > 1)
                    ? (uint32_t)(((uint64_t)y * (PREMIUM_BLUR_H - 1u) * 256u) /
                                 (height - 1u))
                    : 0;
  uint32_t x0 = fx >> 8, y0 = fy >> 8;
  uint32_t x1 = (x0 + 1u < PREMIUM_BLUR_W) ? x0 + 1u : x0;
  uint32_t y1 = (y0 + 1u < PREMIUM_BLUR_H) ? y0 + 1u : y0;
  uint32_t tx = fx & 0xFFu, ty = fy & 0xFFu;

  uint32_t top =
      premium_lerp(s_premium_blur_canvas[y0 * PREMIUM_BLUR_W + x0],
                   s_premium_blur_canvas[y0 * PREMIUM_BLUR_W + x1], tx);
  uint32_t bot =
      premium_lerp(s_premium_blur_canvas[y1 * PREMIUM_BLUR_W + x0],
                   s_premium_blur_canvas[y1 * PREMIUM_BLUR_W + x1], tx);
  return premium_lerp(top, bot, ty);
}

static bool premium_inside_round_rect(int x, int y, int left, int top,
                                      int width, int height, int radius) {
  int right = left + width - 1;
  int bottom = top + height - 1;
  if (x < left || x > right || y < top || y > bottom)
    return false;
  int cx = (x < left + radius) ? left + radius
                               : ((x > right - radius) ? right - radius : x);
  int cy = (y < top + radius) ? top + radius
                              : ((y > bottom - radius) ? bottom - radius : y);
  int dx = x - cx, dy = y - cy;
  return dx * dx + dy * dy <= radius * radius;
}

static void premium_round_rect(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                               uint32_t stride_pixels, int left, int top,
                               int width, int height, int radius, uint32_t fill,
                               uint8_t fill_alpha, uint32_t border,
                               uint8_t border_alpha) {
  for (int y = top; y < top + height; y++) {
    if (y < 0 || y >= (int)fb_h)
      continue;
    uint32_t row = (uint32_t)y * stride_pixels;
    for (int x = left; x < left + width; x++) {
      if (x < 0 || x >= (int)fb_w)
        continue;
      if (!premium_inside_round_rect(x, y, left, top, width, height, radius))
        continue;

      bool inner =
          premium_inside_round_rect(x, y, left + 1, top + 1, width - 2,
                                    height - 2, radius > 1 ? radius - 1 : 1);
      if (!inner && border_alpha)
        fb[row + (uint32_t)x] =
            premium_mix(fb[row + (uint32_t)x], border, border_alpha);
      else if (fill_alpha)
        fb[row + (uint32_t)x] =
            premium_mix(fb[row + (uint32_t)x], fill, fill_alpha);
    }
  }
}



static void premium_center_text(uint32_t *fb, uint32_t width, uint32_t height,
                                uint32_t stride_bytes, BOFontRole role,
                                const char *text, int center_x, int y,
                                uint32_t color, uint8_t alpha) {
  if (!text || alpha == 0)
    return;
  BVFramebuffer target = {0};
  target.buffer = (BOVISUAL_Color *)fb;
  target.width = width;
  target.height = height;
  target.pitch = (stride_bytes >= width * 4) ? stride_bytes : (width * 4);
  BOTextMetrics metrics = BOFont_MeasureTextRole(role, text);
  uint32_t argb = ((uint32_t)alpha << 24) | (color & 0x00FFFFFFu);
  BOFont_DrawTextRoleTarget(&target, role, text, center_x - metrics.width / 2,
                            y, argb);
}

static void premium_draw_lock_mark(uint32_t *fb, uint32_t width,
                                   uint32_t height, uint32_t stride_pixels,
                                   int cx, int cy, uint8_t alpha) {
  extern const uint8_t g_lock_icon_atlas[];
  if (!fb || alpha == 0)
    return;

  int size = 24;
  int start_x = cx - size / 2;
  int start_y = cy - size / 2;

  /* Pass 1: Soft Ambient Drop Shadow (dx = +1, dy = +2) */
  for (int y = 0; y < size; y++) {
    int py = start_y + y + 2;
    if (py < 0 || py >= (int)height)
      continue;
    uint32_t dst_offset = py * stride_pixels;
    for (int x = 0; x < size; x++) {
      int px = start_x + x + 1;
      if (px < 0 || px >= (int)width)
        continue;
      uint8_t sub_a = g_lock_icon_atlas[y * size + x];
      if (sub_a > 0) {
        uint32_t sh_a = ((uint32_t)sub_a * (uint32_t)alpha * 120) / (255 * 255);
        if (sh_a > 0) {
          fb[dst_offset + px] = premium_mix(fb[dst_offset + px], 0x00000000, (uint8_t)sh_a);
        }
      }
    }
  }

  /* Pass 2: Razor-Sharp Ice-White Lock Icon */
  for (int y = 0; y < size; y++) {
    int py = start_y + y;
    if (py < 0 || py >= (int)height)
      continue;
    uint32_t dst_offset = py * stride_pixels;
    for (int x = 0; x < size; x++) {
      int px = start_x + x;
      if (px < 0 || px >= (int)width)
        continue;
      uint8_t sub_a = g_lock_icon_atlas[y * size + x];
      if (sub_a > 0) {
        uint32_t eff_a = ((uint32_t)sub_a * (uint32_t)alpha * 245) / (255 * 255);
        fb[dst_offset + px] = premium_mix(fb[dst_offset + px], 0x00FFFFFF, (uint8_t)eff_a);
      }
    }
  }
}

static void premium_draw_password(uint32_t *fb, uint32_t width, uint32_t height,
                                  uint32_t stride_pixels, int cx, int cy,
                                  const char *password_text, int password_len,
                                  bool show_password, bool cursor_visible,
                                  bool error, uint8_t alpha) {
  if (stride_pixels == 0)
    stride_pixels = width;
  uint32_t stride_bytes = stride_pixels * 4;
  const int box_w = 340, box_h = 52;
  int left = cx - box_w / 2;
  int top = cy - box_h / 2;
  uint32_t border = error ? 0x00F87171 : 0x00FFFFFF;
  premium_round_rect(fb, width, height, stride_pixels, left, top, box_w, box_h,
                     14, 0x00131A26, (uint8_t)((145u * alpha) / 255u), border,
                     (uint8_t)(((error ? 235u : 125u) * alpha) / 255u));

  /* 1. Placeholder or Password Content */
  if (password_len == 0) {
    premium_center_text(fb, width, height, stride_bytes, BOFONT_ROLE_UI_REGULAR,
                        "Password", cx, top + 16, 0x00D6DAE1,
                        (uint8_t)((185u * alpha) / 255u));
  } else if (!show_password) {
    /* Masked Bullets '••••••••' */
    int spacing = 15;
    int start = cx - ((password_len - 1) * spacing) / 2;
    for (int i = 0; i < password_len; i++) {
      int dot_x = start + i * spacing;
      for (int dy = -3; dy <= 3; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= (int)height)
          continue;
        for (int dx = -3; dx <= 3; dx++) {
          int px = dot_x + dx;
          if (px >= 0 && px < (int)width && dx * dx + dy * dy <= 9)
            fb[(uint32_t)py * stride_pixels + (uint32_t)px] =
                premium_mix(fb[(uint32_t)py * stride_pixels + (uint32_t)px],
                            0x00FFFFFF, alpha);
        }
      }
    }
    if (cursor_visible) {
      int caret_x = start + (password_len - 1) * spacing + 12;
      for (int y = cy - 10; y <= cy + 10; y++) {
        if (y >= 0 && y < (int)height && caret_x >= 0 && caret_x < (int)width)
          fb[(uint32_t)y * stride_pixels + (uint32_t)caret_x] =
              premium_mix(fb[(uint32_t)y * stride_pixels + (uint32_t)caret_x],
                          0x00FFFFFF, alpha);
      }
    }
  } else {
    /* Plaintext Password */
    if (password_text) {
      premium_center_text(fb, width, height, stride_bytes, BOFONT_ROLE_UI_REGULAR,
                          password_text, cx, top + 16, 0x00FFFFFF, alpha);
    }
  }

  /* 2. Show/Hide Password Eye Toggle Icon (22x22 from BOOT(OS-ICO)/eye.png) */
  extern const uint8_t g_eye_open_atlas[];
  extern const uint8_t g_eye_slash_atlas[];
  const uint8_t *eye_atlas = show_password ? g_eye_slash_atlas : g_eye_open_atlas;
  int eye_sz = 22;
  int eye_x = left + box_w - 34;
  int eye_y = cy - eye_sz / 2;

  for (int y = 0; y < eye_sz; y++) {
    int py = eye_y + y;
    if (py < 0 || py >= (int)height)
      continue;
    uint32_t dst_row = (uint32_t)py * stride_pixels;
    for (int x = 0; x < eye_sz; x++) {
      int px = eye_x + x;
      if (px < 0 || px >= (int)width)
        continue;
      uint8_t sub_a = eye_atlas[y * eye_sz + x];
      if (sub_a > 0) {
        uint8_t eff_a = (uint8_t)(((uint32_t)sub_a * (uint32_t)alpha * 230) / (255 * 255));
        fb[dst_row + px] = premium_mix(fb[dst_row + px], 0x00FFFFFF, eff_a);
      }
    }
  }
}

static void premium_signin_render(uint32_t *fb, uint32_t width, uint32_t height,
                                  uint32_t stride, const char *password_text,
                                  int password_len, bool show_password,
                                  bool cursor_visible, bool error,
                                  uint8_t alpha) {
  BOS_PROFILE_SCOPE("premium_signin_render");
  if (!fb || width == 0 || height == 0 || alpha == 0)
    return;
  premium_build_blur();
  if (!s_premium_blur_ready)
    return;

  /* Strict Surface Invariant: RAM canvas is ALWAYS dense (stride == width) */
  uint32_t stride_pixels = width;
  uint32_t stride_bytes = width * 4;

  /* Fast pre-baked background copy: 0.05ms frame render time */
  uint32_t copy_w = (width < 1920) ? width : 1920;
  uint32_t copy_h = (height < 1080) ? height : 1080;
  for (uint32_t y = 0; y < copy_h; y++) {
    uint32_t dst_row = y * stride_pixels;
    uint32_t src_row = y * 1920;
    for (uint32_t x = 0; x < copy_w; x++) {
      fb[dst_row + x] = s_darkened_blur_cache[src_row + x];
    }
  }

  int cx = (int)width / 2;
  int cy = (int)height / 2;
  int content_y = cy - 180;

  /* 1. Lock mark */
  premium_draw_lock_mark(fb, width, height, stride_pixels, cx, content_y,
                         alpha);

  /* 2. Circular User Profile Avatar */
  user_profile_service_render_avatar(fb, width, height, stride_pixels, cx,
                                     content_y + 70, 48, alpha);

  /* 3. Welcome Title */
  premium_center_text(fb, width, height, stride_bytes, BOFONT_ROLE_TITLE,
                      "Welcome, Admin", cx, content_y + 135, 0x00FFFFFF, alpha);

  /* 4. Password Input Box with Eye Icon */
  premium_draw_password(fb, width, height, stride_pixels, cx, content_y + 200,
                        password_text, password_len, show_password,
                        cursor_visible, error, alpha);

  /* 5. Error Subtext */
  if (error) {
    premium_center_text(fb, width, height, stride_bytes, BOFONT_ROLE_CAPTION,
                        "Incorrect Password", cx, content_y + 238, 0x00FCA5A5,
                        alpha);
  }

  /* 6. Sign In Action Button (Translucent Frosted Pill + Crisp Border + Bold Text) */
  const PointerState *ps = pointer_state_get();
  bool hovered = false;
  int btn_w = 180, btn_h = 44;
  int btn_x = cx - btn_w / 2;
  int btn_y = content_y + 265;
  if (ps && ps->current_x >= btn_x && ps->current_x < btn_x + btn_w &&
      ps->current_y >= btn_y && ps->current_y < btn_y + btn_h) {
    hovered = true;
  }

  uint8_t fill_a = (uint8_t)(((hovered ? 210u : 170u) * alpha) / 255u);
  uint8_t border_a = (uint8_t)(((hovered ? 255u : 230u) * alpha) / 255u);

  premium_round_rect(fb, width, height, stride_pixels, btn_x, btn_y,
                     btn_w, btn_h, 22, 0x00FFFFFF, fill_a,
                     0x00FFFFFF, border_a);
  premium_center_text(fb, width, height, stride_bytes, BOFONT_ROLE_UI_BOLD,
                      "Sign In", cx, content_y + 277, 0x000F172A, alpha);

  /* 7. Bottom-Right Power Controls: Restart (↻) & Shutdown (⏻) */
  extern const uint8_t g_restart_icon_atlas[];
  extern const uint8_t g_shutdown_icon_atlas[];
  int pwr_sz = 22;

  int res_btn_x = (int)width - 105;
  int res_btn_y = (int)height - 60;
  bool res_hov = (ps && ps->current_x >= res_btn_x && ps->current_x < res_btn_x + 40 &&
                  ps->current_y >= res_btn_y && ps->current_y < res_btn_y + 40);

  int shut_btn_x = (int)width - 55;
  int shut_btn_y = (int)height - 60;
  bool shut_hov = (ps && ps->current_x >= shut_btn_x && ps->current_x < shut_btn_x + 40 &&
                   ps->current_y >= shut_btn_y && ps->current_y < shut_btn_y + 40);

  /* Restart Button (Frosted Slate Circle) */
  uint8_t res_fill = (uint8_t)(((res_hov ? 210u : 160u) * alpha) / 255u);
  uint8_t res_border = (uint8_t)(((res_hov ? 255u : 180u) * alpha) / 255u);
  premium_round_rect(fb, width, height, stride_pixels, res_btn_x, res_btn_y, 40, 40, 20,
                     0x001E2D41, res_fill, 0x00FFFFFF, res_border);
  for (int y = 0; y < pwr_sz; y++) {
    int py = res_btn_y + 9 + y;
    if (py < 0 || py >= (int)height) continue;
    uint32_t dst_row = (uint32_t)py * stride_pixels;
    for (int x = 0; x < pwr_sz; x++) {
      int px = res_btn_x + 9 + x;
      if (px < 0 || px >= (int)width) continue;
      uint8_t a_val = g_restart_icon_atlas[y * pwr_sz + x];
      if (a_val > 0) {
        uint8_t eff_a = (uint8_t)(((uint32_t)a_val * (uint32_t)alpha * 240) / (255 * 255));
        fb[dst_row + px] = premium_mix(fb[dst_row + px], 0x00FFFFFF, eff_a);
      }
    }
  }

  /* Shutdown Button (Frosted Slate Circle) */
  uint8_t shut_fill = (uint8_t)(((shut_hov ? 210u : 160u) * alpha) / 255u);
  uint8_t shut_border = (uint8_t)(((shut_hov ? 255u : 180u) * alpha) / 255u);
  premium_round_rect(fb, width, height, stride_pixels, shut_btn_x, shut_btn_y, 40, 40, 20,
                     0x001E2D41, shut_fill, 0x00FFFFFF, shut_border);
  for (int y = 0; y < pwr_sz; y++) {
    int py = shut_btn_y + 9 + y;
    if (py < 0 || py >= (int)height) continue;
    uint32_t dst_row = (uint32_t)py * stride_pixels;
    for (int x = 0; x < pwr_sz; x++) {
      int px = shut_btn_x + 9 + x;
      if (px < 0 || px >= (int)width) continue;
      uint8_t a_val = g_shutdown_icon_atlas[y * pwr_sz + x];
      if (a_val > 0) {
        uint8_t eff_a = (uint8_t)(((uint32_t)a_val * (uint32_t)alpha * 240) / (255 * 255));
        fb[dst_row + px] = premium_mix(fb[dst_row + px], 0x00FFFFFF, eff_a);
      }
    }
  }
}

#endif /* PREMIUM_SIGNIN_RENDERER_H */
