#include "kernel/shell/rook/pages/page_login.h"
#include "bovisual/Include/graphics.h"
#include "bovisual/Include/text.h"
#include "kernel/ame/include/ame.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/drivers/rtc/rtc.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/services/user_profile/user_profile_service.h"
#include "kernel/services/wallpaper/boot_assets.h"
#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/shell/rook/pages/premium_signin_renderer.h"
#include "kernel/ui/bofont/bofont.h"
#include "clock_atlas.h"
#include "clock_atlas.c"

/*
 * ♜ ATOMS OS Login Page (Page 3: ROOK_PAGE_LOGIN)
 * Single Page Architecture: Lock Screen (State 1) -> Sign In (State 2)
 * Driven by ATOMS Motion Engine (AME).
 * 0% Wallpaper reloads, 0% Flicker, 100% Deterministic 60 FPS.
 */

typedef enum {
  LOGIN_STATE_LOCK = 0,
  LOGIN_STATE_TRANSITION,
  LOGIN_STATE_SIGN_IN,
  LOGIN_STATE_AUTH_SUCCESS,
  LOGIN_STATE_PREPARING_DESKTOP
} login_page_state_t;

static rook_page_t s_login_page;
static login_page_state_t s_login_state = LOGIN_STATE_LOCK;
static bool s_login_initialized = false;

/* Decoded PNG Icon Surfaces */
static struct BOSSurface *s_lock_icon_surf = NULL;
static struct BOSSurface *s_ethernet_icon_surf = NULL;
static struct BOSSurface *s_chat_icon_surf = NULL;
static bool s_icons_decoded = false;

/* Transition animation telemetry */
static uint64_t s_trans_elapsed_ms = 0;
static uint8_t s_lock_alpha = 255;
static uint8_t s_signin_alpha = 0;
static uint8_t s_loading_alpha = 0;
static uint64_t s_loading_elapsed_ms = 0;
static int32_t s_password_offset_y = 40;

/* Password input buffer & authentication state */
static char s_password_buf[64] = {0};
static int s_password_len = 0;
static bool s_password_error = false;
static uint64_t s_cursor_blink_ms = 0;

static uint32_t blend_alpha(uint32_t bg_color, uint32_t fg_color,
                            uint8_t alpha) {
  if (alpha == 0)
    return bg_color;
  uint8_t fg_a = (fg_color >> 24) & 0xFF;
  if (fg_a == 0)
    return bg_color;
  uint32_t eff_alpha = ((uint32_t)fg_a * (uint32_t)alpha) / 255;
  if (eff_alpha == 0)
    return bg_color;

  uint32_t r_bg = (bg_color >> 16) & 0xFF;
  uint32_t g_bg = (bg_color >> 8) & 0xFF;
  uint32_t b_bg = bg_color & 0xFF;

  uint32_t r_fg = (fg_color >> 16) & 0xFF;
  uint32_t g_fg = (fg_color >> 8) & 0xFF;
  uint32_t b_fg = fg_color & 0xFF;

  uint32_t inv = 255 - eff_alpha;
  uint32_t r = (r_bg * inv + r_fg * eff_alpha) / 255;
  uint32_t g = (g_bg * inv + g_fg * eff_alpha) / 255;
  uint32_t b = (b_bg * inv + b_fg * eff_alpha) / 255;

  return (r << 16) | (g << 8) | b;
}

static void decode_icons_if_needed(void) {
  s_icons_decoded = true;
}
/* Render 1:1 Native Resolution Pixel-Snapped Razor-Sharp Icon */
static void draw_atlas_icon_centered(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                                     uint32_t stride_pixels, const uint8_t *icon_atlas,
                                     int icon_size,
                                     int cx, int cy, uint8_t alpha, bool add_shadow) {
  if (!fb || !icon_atlas || alpha == 0 || icon_size <= 0)
    return;

  int size = icon_size;
  int start_x = cx - size / 2;
  int start_y = cy - size / 2;

  /* Optional Pass 1: Soft Ambient Shadow (dx = +1, dy = +2) */
  if (add_shadow) {
    for (int y = 0; y < size; y++) {
      int py = start_y + y + 2;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int x = 0; x < size; x++) {
        int px = start_x + x + 1;
        if (px < 0 || px >= (int)fb_w)
          continue;

        uint8_t sub_a = icon_atlas[y * size + x];
        if (sub_a > 0) {
          uint32_t sh_a = ((uint32_t)sub_a * (uint32_t)alpha * 120) / (255 * 255);
          if (sh_a > 0) {
            fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFF000000, (uint8_t)sh_a);
          }
        }
      }
    }
  }

  /* Pass 2: Razor-Sharp Pure White Icon */
  for (int y = 0; y < size; y++) {
    int py = start_y + y;
    if (py < 0 || py >= (int)fb_h)
      continue;
    uint32_t dst_offset = py * stride_pixels;

    for (int x = 0; x < size; x++) {
      int px = start_x + x;
      if (px < 0 || px >= (int)fb_w)
        continue;

      uint8_t sub_a = icon_atlas[y * size + x];
      if (sub_a > 0) {
        uint32_t eff_a = ((uint32_t)sub_a * (uint32_t)alpha * 240) / (255 * 255);
        fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, (uint8_t)eff_a);
      }
    }
  }
}

static uint32_t clock_isqrt(uint32_t n) {
  uint32_t root = 0;
  uint32_t bit = 1U << 30;
  while (bit > n)
    bit >>= 2;
  while (bit != 0) {
    if (n >= root + bit) {
      n -= root + bit;
      root = (root >> 1) + bit;
    } else {
      root >>= 1;
    }
    bit >>= 2;
  }
  return root;
}

static uint32_t dist_sq_seg(int px, int py, int x1, int y1, int x2, int y2) {
  int dx = x2 - x1;
  int dy = y2 - y1;
  int l2 = dx * dx + dy * dy;
  if (l2 == 0) {
    int rx = px - x1;
    int ry = py - y1;
    return rx * rx + ry * ry;
  }
  int t = (px - x1) * dx + (py - y1) * dy;
  if (t <= 0) {
    int rx = px - x1;
    int ry = py - y1;
    return rx * rx + ry * ry;
  }
  if (t >= l2) {
    int rx = px - x2;
    int ry = py - y2;
    return rx * rx + ry * ry;
  }
  int proj_x = x1 + (t * dx) / l2;
  int proj_y = y1 + (t * dy) / l2;
  int rx = px - proj_x;
  int ry = py - proj_y;
  return rx * rx + ry * ry;
}

static uint32_t dist_circle_arc(int px, int py, int cx, int cy, int radius) {
  int dx = px - cx;
  int dy = py - cy;
  int d = clock_isqrt(dx * dx + dy * dy);
  int diff = d - radius;
  return (diff < 0) ? -diff : diff;
}

static inline uint8_t get_atlas_alpha(char ch, int x, int y) {
  if (ch >= '0' && ch <= '9') {
    int d = ch - '0';
    if (x >= 0 && x < CLOCK_DIGIT_W && y >= 0 && y < CLOCK_DIGIT_H) {
      return g_clock_digit_atlas[d][y * CLOCK_DIGIT_W + x];
    }
  } else if (ch == ':') {
    if (x >= 0 && x < CLOCK_COLON_W && y >= 0 && y < CLOCK_DIGIT_H) {
      return g_clock_colon_atlas[y * CLOCK_COLON_W + x];
    }
  }
  return 0;
}

/* Render Apple iOS 17 Condensed Tall Frosted Glass Lock Screen Clock */
static void draw_large_time(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                            uint32_t stride_pixels, int cx, int cy,
                            const char *time_str, uint8_t alpha) {
  if (!fb || !time_str || alpha == 0)
    return;

  int len = 0;
  while (time_str[len])
    len++;

  int digit_w = CLOCK_DIGIT_W;
  int digit_h = CLOCK_DIGIT_H;
  int spacing = 8;
  int colon_w = CLOCK_COLON_W;

  int total_w = 0;
  for (int i = 0; i < len; i++) {
    total_w += (time_str[i] == ':') ? colon_w : digit_w;
    if (i < len - 1)
      total_w += spacing;
  }

  int start_x = cx - total_w / 2;

  /* Pass 1: Soft Ambient Drop Shadow (dx = +2, dy = +3) */
  int curr_x = start_x;
  for (int i = 0; i < len; i++) {
    char ch = time_str[i];
    int w = (ch == ':') ? colon_w : digit_w;

    for (int y = 0; y < digit_h; y++) {
      int py = cy + y + 3;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int x = 0; x < w; x++) {
        int px = curr_x + x + 2;
        if (px < 0 || px >= (int)fb_w)
          continue;

        uint8_t sub_alpha = get_atlas_alpha(ch, x, y);
        if (sub_alpha > 0) {
          uint32_t sh_a = ((uint32_t)sub_alpha * (uint32_t)alpha * 90) / (255 * 255);
          if (sh_a > 0) {
            fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFF000000, (uint8_t)sh_a);
          }
        }
      }
    }
    curr_x += w + spacing;
  }

  /* Pass 2: Apple Translucent Frosted Glass Glyphs (0xFFFFFFFF, ~60% opacity) */
  curr_x = start_x;
  for (int i = 0; i < len; i++) {
    char ch = time_str[i];
    int w = (ch == ':') ? colon_w : digit_w;

    for (int y = 0; y < digit_h; y++) {
      int py = cy + y;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int x = 0; x < w; x++) {
        int px = curr_x + x;
        if (px < 0 || px >= (int)fb_w)
          continue;

        uint8_t sub_alpha = get_atlas_alpha(ch, x, y);
        if (sub_alpha > 0) {
          uint32_t final_alpha = ((uint32_t)sub_alpha * (uint32_t)alpha * 155) / (255 * 255);
          fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, (uint8_t)final_alpha);
        }
      }
    }
    curr_x += w + spacing;
  }
}

/* Render Apple iOS 17 TrueType Anti-Aliased Date Header (e.g. "SATURDAY, AUG 15") */
static void draw_date_header(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                             uint32_t stride_pixels, int cx, int cy,
                             const char *date_str, uint8_t alpha) {
  if (!fb || !date_str || alpha == 0)
    return;

  int len = 0;
  int total_w = 0;
  while (date_str[len]) {
    char ch = date_str[len];
    if (ch >= 32 && ch <= 126) {
      total_w += g_date_font_widths[ch - 32];
    } else {
      total_w += 10;
    }
    len++;
  }

  int start_x = cx - total_w / 2;

  /* Pass 1: Soft Ambient Drop Shadow (dx = +1, dy = +2) */
  int curr_x = start_x;
  for (int i = 0; i < len; i++) {
    char ch = date_str[i];
    if (ch < 32 || ch > 126) {
      curr_x += 10;
      continue;
    }
    int glyph_idx = ch - 32;
    int w = g_date_font_widths[glyph_idx];

    for (int y = 0; y < DATE_FONT_H; y++) {
      int py = cy + y + 2;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int x = 0; x < w; x++) {
        int px = curr_x + x + 1;
        if (px < 0 || px >= (int)fb_w)
          continue;

        uint8_t sub_a = g_date_font_atlas[glyph_idx][y * DATE_FONT_W + x];
        if (sub_a > 0) {
          uint32_t sh_a = ((uint32_t)sub_a * (uint32_t)alpha * 90) / (255 * 255);
          if (sh_a > 0) {
            fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFF000000, (uint8_t)sh_a);
          }
        }
      }
    }
    curr_x += w;
  }

  /* Pass 2: Apple Translucent Frosted Glass Glyphs (0xFFFFFFFF, ~60% opacity) */
  curr_x = start_x;
  for (int i = 0; i < len; i++) {
    char ch = date_str[i];
    if (ch < 32 || ch > 126) {
      curr_x += 10;
      continue;
    }
    int glyph_idx = ch - 32;
    int w = g_date_font_widths[glyph_idx];

    for (int y = 0; y < DATE_FONT_H; y++) {
      int py = cy + y;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int x = 0; x < w; x++) {
        int px = curr_x + x;
        if (px < 0 || px >= (int)fb_w)
          continue;

        uint8_t sub_a = g_date_font_atlas[glyph_idx][y * DATE_FONT_W + x];
        if (sub_a > 0) {
          uint32_t final_alpha = ((uint32_t)sub_a * (uint32_t)alpha * 155) / (255 * 255);
          fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, (uint8_t)final_alpha);
        }
      }
    }
    curr_x += w;
  }
}

/* Render Modern Glass Rounded Container for Bottom Icons */
static void draw_rounded_container(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                                   uint32_t stride_pixels, int cx, int cy,
                                   int size, int radius, uint8_t alpha) {
  extern void audit_log_draw(const char *func, int x, int y, int w, int h,
                             int r, uint32_t color);

  audit_log_draw("draw_rounded_container", cx, cy, size, size, radius, alpha);
  if (alpha == 0)
    return;

  int half = size / 2;
  int x1 = cx - half;
  int y1 = cy - half;
  int x2 = cx + half;
  int y2 = cy + half;

  for (int py = y1; py <= y2; py++) {
    if (py < 0 || py >= (int)fb_h)
      continue;
    uint32_t dst_offset = py * stride_pixels;

    for (int px = x1; px <= x2; px++) {
      if (px < 0 || px >= (int)fb_w)
        continue;

      int dx = (px < x1 + radius)
                   ? (x1 + radius - px)
                   : ((px > x2 - radius) ? (px - (x2 - radius)) : 0);
      int dy = (py < y1 + radius)
                   ? (y1 + radius - py)
                   : ((py > y2 - radius) ? (py - (y2 - radius)) : 0);
      int d_corner =
          (dx > 0 && dy > 0) ? (int)clock_isqrt(dx * dx + dy * dy) : 0;

      if (d_corner > radius)
        continue;

      // Translucent glass fill
      uint8_t bg_a = (uint8_t)((35 * alpha) / 255);
      fb[dst_offset + px] = blend_alpha(
          fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)bg_a << 24), bg_a);

      // Thin crisp white border outline
      bool is_border = false;
      if (dx > 0 && dy > 0) {
        if (d_corner >= radius - 2 && d_corner <= radius)
          is_border = true;
      } else {
        if (px == x1 || px == x2 || py == y1 || py == y2)
          is_border = true;
      }

      if (is_border) {
        uint8_t border_a = (uint8_t)((180 * alpha) / 255);
        fb[dst_offset + px] =
            blend_alpha(fb[dst_offset + px],
                        0x00FFFFFF | ((uint32_t)border_a << 24), border_a);
      }
    }
  }
}



/* Vector/BOFont character renderer for Date & Text */
static void draw_custom_text(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                             uint32_t stride_pixels, int cx, int y,
                             const char *str, uint32_t color, bool large,
                             uint8_t alpha) {
  if (!str || alpha == 0)
    return;

  BVFramebuffer target_fb;
  target_fb.buffer = fb;
  target_fb.width = fb_w;
  target_fb.height = fb_h;
  target_fb.pitch = stride_pixels * 4;

  BOFontRole role = large ? BOFONT_ROLE_TITLE : BOFONT_ROLE_CAPTION;
  BOTextMetrics tm = BOFont_MeasureTextRole(role, str);
  if (tm.width > 0) {
    int text_x = cx - tm.width / 2;
    BOFont_DrawTextRoleTarget(&target_fb, role, str, text_x, y, color);
    return;
  }

  int len = 0;
  while (str[len])
    len++;

  int char_w = large ? 14 : 9;
  int char_h = large ? 24 : 15;
  int spacing = 2;

  int total_w = len * char_w + (len - 1) * spacing;
  int start_x = cx - total_w / 2;

  for (int i = 0; i < len; i++) {
    char ch = str[i];
    int char_x = start_x + i * (char_w + spacing);

    if (ch == ' ')
      continue;

    for (int cy_pos = 0; cy_pos < char_h; cy_pos++) {
      int py = y + cy_pos;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;

      for (int cx_pos = 0; cx_pos < char_w; cx_pos++) {
        int px = char_x + cx_pos;
        if (px < 0 || px >= (int)fb_w)
          continue;

        bool stroke = false;
        int norm_x = cx_pos * 8 / char_w;
        int norm_y = cy_pos * 8 / char_h;

        if (ch >= 'A' && ch <= 'Z') {
          if (norm_x == 0 || norm_x == 7 || norm_y == 0 || norm_y == 4 ||
              norm_y == 7)
            stroke = true;
        } else if (ch >= 'a' && ch <= 'z') {
          if (norm_x == 0 || norm_x == 7 || norm_y == 3 || norm_y == 7)
            stroke = true;
        } else if (ch >= '0' && ch <= '9') {
          if (norm_x == 0 || norm_x == 7 || norm_y == 0 || norm_y == 7)
            stroke = true;
        } else if (ch == ',') {
          if (norm_x >= 3 && norm_x <= 5 && norm_y >= 6 && norm_y <= 7)
            stroke = true;
        }

        if (stroke) {
          uint32_t eff_color = color | ((uint32_t)alpha << 24);
          fb[dst_offset + px] =
              blend_alpha(fb[dst_offset + px], eff_color, alpha);
        }
      }
    }
  }
}

/* Render Premium Frosted Glass Desktop Loading Experience */
static void draw_desktop_loading_experience(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                                            uint32_t stride_pixels, int cx, int cy,
                                            uint8_t alpha, uint64_t elapsed_ms) {
  if (!fb || alpha == 0) return;

  /* 1. Translucent Frosted Glass Capsule (360px wide, 80px high, radius 18px) */
  int box_w = 360;
  int box_h = 80;
  int x1 = cx - box_w / 2;
  int x2 = cx + box_w / 2;
  int y1 = cy - box_h / 2;
  int y2 = cy + box_h / 2;
  int radius = 18;

  /* Pass 1: Soft Ambient Drop Shadow */
  for (int py = y1 + 4; py < y2 + 14; py++) {
    if (py < 0 || py >= (int)fb_h) continue;
    uint32_t dst_offset = py * stride_pixels;
    for (int px = x1 - 4; px < x2 + 4; px++) {
      if (px < 0 || px >= (int)fb_w) continue;
      int dx = 0, dy = 0;
      if (px < x1 + radius) dx = (x1 + radius) - px;
      else if (px > x2 - radius) dx = px - (x2 - radius);
      if (py < y1 + radius) dy = (y1 + radius) - py;
      else if (py > y2 - radius) dy = py - (y2 - radius);

      if (dx > 0 && dy > 0) {
        if (clock_isqrt(dx * dx + dy * dy) > (uint32_t)radius) continue;
      }
      uint8_t sh_a = (uint8_t)((60 * alpha) / 255);
      fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFF000000, sh_a);
    }
  }

  /* Pass 2: Glass Body & Outline */
  for (int py = y1; py < y2; py++) {
    if (py < 0 || py >= (int)fb_h) continue;
    uint32_t dst_offset = py * stride_pixels;
    for (int px = x1; px < x2; px++) {
      if (px < 0 || px >= (int)fb_w) continue;
      int dx = 0, dy = 0;
      if (px < x1 + radius) dx = (x1 + radius) - px;
      else if (px > x2 - radius) dx = px - (x2 - radius);
      if (py < y1 + radius) dy = (y1 + radius) - py;
      else if (py > y2 - radius) dy = py - (y2 - radius);

      uint32_t d_corner = 0;
      if (dx > 0 && dy > 0) {
        d_corner = clock_isqrt(dx * dx + dy * dy);
        if (d_corner > (uint32_t)radius) continue;
      }

      /* Frosted glass tint (pure modern translucent acrylic) */
      uint8_t glass_a = (uint8_t)((36 * alpha) / 255);
      if (py < y1 + 20) glass_a += (uint8_t)((14 * alpha) / 255); /* soft top highlight */

      fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)glass_a << 24), glass_a);

      /* Crisp 1px White Border Outline */
      bool is_border = false;
      if (dx > 0 && dy > 0) {
        if (d_corner >= (uint32_t)(radius - 1) && d_corner <= (uint32_t)radius) is_border = true;
      } else {
        if (px == x1 || px == x2 - 1 || py == y1 || py == y2 - 1) is_border = true;
      }

      if (is_border) {
        uint8_t border_a = (uint8_t)((110 * alpha) / 255);
        fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)border_a << 24), border_a);
      }
    }
  }

  /* 2. Subtle Orbital Dot Spinner (radius = 12px, centered at cx - 120, cy) */
  int spin_cx = cx - 120;
  int spin_cy = cy;
  static const int8_t ring_dx[12] = { 12, 10, 6, 0, -6, -10, -12, -10, -6, 0, 6, 10 };
  static const int8_t ring_dy[12] = { 0, 6, 10, 12, 10, 6, 0, -6, -10, -12, -10, -6 };
  int active_idx = (int)((elapsed_ms / 75) % 12);

  for (int i = 0; i < 12; i++) {
    int dot_x = spin_cx + ring_dx[i];
    int dot_y = spin_cy + ring_dy[i];
    int dist = (i - active_idx + 12) % 12; /* 0 = leader, 11 = tail */
    uint8_t dot_alpha = (uint8_t)(((255 - dist * 18) * alpha) / 255);
    if (dist > 8) dot_alpha = (uint8_t)((30 * alpha) / 255);

    /* Draw 3x3 anti-aliased dot bead */
    for (int dy = -1; dy <= 1; dy++) {
      int py = dot_y + dy;
      if (py < 0 || py >= (int)fb_h) continue;
      uint32_t dst_offset = py * stride_pixels;
      for (int dx = -1; dx <= 1; dx++) {
        int px = dot_x + dx;
        if (px < 0 || px >= (int)fb_w) continue;
        uint8_t sub_a = (dx == 0 && dy == 0) ? dot_alpha : (uint8_t)((dot_alpha * 160) / 255);
        fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, sub_a);
      }
    }
  }

  /* 3. Minimal Clean Typography: "Preparing your desktop…" */
  const char *text = "Preparing your desktop...";
  draw_custom_text(fb, fb_w, fb_h, stride_pixels, cx + 25, cy - 8, text, 0xFFFFFFFF, false, alpha);
}

/* Render Password Entry Input Box */
static void draw_password_box(uint32_t *fb, uint32_t fb_w, uint32_t fb_h,
                              uint32_t stride_pixels, int cx, int cy,
                              int password_len, bool cursor_visible,
                              bool has_error, uint8_t alpha) {
  if (!fb || alpha == 0)
    return;

  int box_w = 260;
  int box_h = 44;
  int start_x = cx - box_w / 2;
  int start_y = cy - box_h / 2;

  for (int y = 0; y < box_h; y++) {
    int py = start_y + y;
    if (py < 0 || py >= (int)fb_h)
      continue;
    uint32_t dst_offset = py * stride_pixels;
    for (int x = 0; x < box_w; x++) {
      int px = start_x + x;
      if (px < 0 || px >= (int)fb_w)
        continue;

      bool is_border = (x == 0 || x == box_w - 1 || y == 0 || y == box_h - 1);
      uint32_t fg;
      uint8_t eff_alpha;

      if (has_error) {
        fg = is_border ? 0x00EF4444 : 0x001F0909;
        eff_alpha = is_border ? (uint8_t)((240 * alpha) / 255)
                              : (uint8_t)((180 * alpha) / 255);
      } else {
        fg = is_border ? 0x0094A3B8 : 0x000F172A;
        eff_alpha = is_border ? (uint8_t)((200 * alpha) / 255)
                              : (uint8_t)((160 * alpha) / 255);
      }
      uint32_t fg_with_a = fg | ((uint32_t)eff_alpha << 24);

      fb[dst_offset + px] =
          blend_alpha(fb[dst_offset + px], fg_with_a, eff_alpha);
    }
  }

  int dot_spacing = 14;
  int total_dots_w = (password_len > 0) ? (password_len * dot_spacing) : 0;
  int dots_start_x = cx - total_dots_w / 2;

  for (int i = 0; i < password_len; i++) {
    int dot_cx = dots_start_x + i * dot_spacing + dot_spacing / 2;
    int dot_cy = cy;
    for (int dy = -3; dy <= 3; dy++) {
      int py = dot_cy + dy;
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;
      for (int dx = -3; dx <= 3; dx++) {
        int px = dot_cx + dx;
        if (px < 0 || px >= (int)fb_w)
          continue;
        if (dx * dx + dy * dy <= 3 * 3) {
          fb[dst_offset + px] =
              blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, alpha);
        }
      }
    }
  }

  if (cursor_visible) {
    int cursor_x = (password_len > 0)
                       ? (dots_start_x + password_len * dot_spacing + 4)
                       : cx;
    int cursor_y_top = cy - 10;
    int cursor_y_bot = cy + 10;

    for (int py = cursor_y_top; py <= cursor_y_bot; py++) {
      if (py < 0 || py >= (int)fb_h)
        continue;
      uint32_t dst_offset = py * stride_pixels;
      for (int px = cursor_x - 1; px <= cursor_x + 1; px++) {
        if (px < 0 || px >= (int)fb_w)
          continue;
        fb[dst_offset + px] =
            blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, alpha);
      }
    }
  }
}

static int page_login_on_create(rook_page_t *page) {
  (void)page;
  wallpaper_service_init();
  user_profile_service_init();
  decode_icons_if_needed();
  s_login_state = LOGIN_STATE_LOCK;
  s_lock_alpha = 255;
  s_signin_alpha = 0;
  s_password_offset_y = 40;
  s_trans_elapsed_ms = 0;
  s_password_len = 0;
  s_password_error = false;
  s_login_initialized = true;
  return 0;
}

static int page_login_on_init(rook_page_t *page) {
  (void)page;
  s_login_state = LOGIN_STATE_LOCK;
  s_lock_alpha = 255;
  s_signin_alpha = 0;
  s_password_offset_y = 40;
  s_trans_elapsed_ms = 0;
  s_password_len = 0;
  s_password_error = false;
  return 0;
}

static int page_login_on_load(rook_page_t *page) {
  (void)page;
  return 0;
}

static int page_login_on_enter(rook_page_t *page) {
  (void)page;
  wallpaper_service_init();
  decode_icons_if_needed();
  s_login_state = LOGIN_STATE_LOCK;
  s_lock_alpha = 255;
  s_signin_alpha = 0;
  s_loading_alpha = 0;
  s_loading_elapsed_ms = 0;
  s_password_offset_y = 40;
  s_trans_elapsed_ms = 0;
  s_password_len = 0;
  s_password_error = false;

  rook_invalidate_full();
  return 0;
}


static bool s_show_password = false;

static int page_login_on_update(rook_page_t *page, uint64_t delta_ms) {
  (void)page;
  s_cursor_blink_ms += delta_ms;
  wallpaper_service_update(delta_ms);

  const PointerState *ps = pointer_state_get();
  bool mouse_clicked = (ps && (ps->button_just_pressed & 0x01));

  KeyboardEvent key_evt;
  bool key_pressed = false;

  if (s_login_state == LOGIN_STATE_LOCK) {
    if (key_pressed || mouse_clicked) {
      s_login_state = LOGIN_STATE_TRANSITION;
      s_trans_elapsed_ms = 0;
    }
  }

  while (keyboard_poll_event(&key_evt)) {
    key_pressed = true;

    if (s_login_state == LOGIN_STATE_LOCK) {
      if (key_evt.pressed) {
        s_login_state = LOGIN_STATE_TRANSITION;
        s_trans_elapsed_ms = 0;
        break;
      }
    } else if (s_login_state == LOGIN_STATE_SIGN_IN) {
      bool submit = false;
      if (key_evt.pressed &&
          (key_evt.keycode == 0x1C || key_evt.ascii == '\n' ||
           key_evt.ascii == '\r')) {
        submit = true;
      }

      if (submit) {
        if (s_password_len > 0 && strcmp(s_password_buf, "admin123") == 0) {
          s_password_error = false;
          s_login_state = LOGIN_STATE_AUTH_SUCCESS;
          s_trans_elapsed_ms = 0;
        } else {
          s_password_error = true;
          s_password_len = 0;
          s_password_buf[0] = '\0';
        }
      } else if (key_evt.pressed) {
        if (key_evt.keycode == 0x0E || key_evt.ascii == '\b') {
          if (s_password_len > 0) {
            s_password_len--;
            s_password_buf[s_password_len] = '\0';
          }
          s_password_error = false;
        } else if (key_evt.keycode == 0x3B || (key_evt.ctrl && (key_evt.ascii == 'p' || key_evt.ascii == 'P'))) {
          /* F1 or Ctrl+P: Toggle Password Visibility */
          s_show_password = !s_show_password;
        } else if (key_evt.ascii >= 32 && key_evt.ascii <= 126) {
          if (s_password_len < 63) {
            s_password_buf[s_password_len++] = key_evt.ascii;
            s_password_buf[s_password_len] = '\0';
          }
          s_password_error = false;
        }
      }
    }
  }

  uint32_t scr_w = rook_get_width();
  uint32_t scr_h = rook_get_height();

  if (mouse_clicked && ps) {
    /* Corner Power Controls (Only active in Sign-In state) */
    if (s_login_state == LOGIN_STATE_SIGN_IN || s_signin_alpha > 0) {
      if (ps->current_x >= (int)scr_w - 105 && ps->current_x < (int)scr_w - 65 &&
          ps->current_y >= (int)scr_h - 60 && ps->current_y < (int)scr_h - 20) {
        extern void system_reboot(void);
        system_reboot();
      } else if (ps->current_x >= (int)scr_w - 55 && ps->current_x < (int)scr_w - 15 &&
                 ps->current_y >= (int)scr_h - 60 && ps->current_y < (int)scr_h - 20) {
        extern void system_shutdown(void);
        system_shutdown();
      }
    } else if (s_login_state == LOGIN_STATE_LOCK) {
      s_login_state = LOGIN_STATE_TRANSITION;
      s_trans_elapsed_ms = 0;
    }
  }

  if (s_login_state == LOGIN_STATE_TRANSITION) {
    s_trans_elapsed_ms += delta_ms;

    float p = (float)s_trans_elapsed_ms / 400.0f;
    if (p > 1.0f)
      p = 1.0f;
    float inv = 1.0f - p;
    float ease_p = 1.0f - (inv * inv * inv);

    s_lock_alpha = (uint8_t)(255.0f * (1.0f - ease_p));
    s_signin_alpha = (uint8_t)(255.0f * ease_p);
    s_password_offset_y = (int32_t)(40.0f * (1.0f - ease_p));

    if (p >= 1.0f) {
      s_login_state = LOGIN_STATE_SIGN_IN;
      s_lock_alpha = 0;
      s_signin_alpha = 255;
      s_password_offset_y = 0;
    }
  } else if (s_login_state == LOGIN_STATE_SIGN_IN) {
    if (mouse_clicked && ps) {
      int cx = (int)scr_w / 2;
      int cy = (int)scr_h / 2;

      /* 1. Eye Toggle Icon Hit Test: cx + 125 .. cx + 155, cy + 20 - 15 .. cy + 20 + 15 */
      if (ps->current_x >= cx + 125 && ps->current_x < cx + 155 &&
          ps->current_y >= cy + 5 && ps->current_y < cy + 35) {
        s_show_password = !s_show_password;
      }

      /* 2. Submit Sign-In Button Hit Test */
      int button_y = cy + 85;
      if (ps->current_x >= cx - 90 && ps->current_x < cx + 90 &&
          ps->current_y >= button_y && ps->current_y < button_y + 44) {
        if (s_password_len > 0 && strcmp(s_password_buf, "admin123") == 0) {
          s_password_error = false;
          s_login_state = LOGIN_STATE_AUTH_SUCCESS;
          s_trans_elapsed_ms = 0;
        } else {
          s_password_error = true;
          s_password_len = 0;
          s_password_buf[0] = '\0';
        }
      }
    }
  } else if (s_login_state == LOGIN_STATE_AUTH_SUCCESS) {
    s_trans_elapsed_ms += delta_ms;
    uint64_t elapsed = (s_trans_elapsed_ms > 200) ? 200 : s_trans_elapsed_ms;
    s_signin_alpha = (uint8_t)(255u - (elapsed * 255u) / 200u);
    if (s_trans_elapsed_ms >= 200) {
      s_login_state = LOGIN_STATE_PREPARING_DESKTOP;
      s_loading_elapsed_ms = 0;
      s_loading_alpha = 0;
    }
  } else if (s_login_state == LOGIN_STATE_PREPARING_DESKTOP) {
    s_loading_elapsed_ms += delta_ms;
    uint64_t fade_in = (s_loading_elapsed_ms > 200) ? 200 : s_loading_elapsed_ms;
    s_loading_alpha = (uint8_t)((fade_in * 255u) / 200u);
    if (s_loading_elapsed_ms >= 200) {
      rook_goto(ROOK_PAGE_DESKTOP);
    }
  }

  bool state_changed = false;
  if (key_pressed || mouse_clicked) state_changed = true;
  if (s_login_state == LOGIN_STATE_TRANSITION || s_login_state == LOGIN_STATE_AUTH_SUCCESS || s_login_state == LOGIN_STATE_PREPARING_DESKTOP) state_changed = true;

  static bool s_last_caret_blink = false;
  bool caret_blink = ((s_cursor_blink_ms / 500) % 2 == 0);
  if (s_login_state == LOGIN_STATE_SIGN_IN && caret_blink != s_last_caret_blink) {
    s_last_caret_blink = caret_blink;
    state_changed = true;
  }

  static int s_last_min = -1;
  RTCDateTime dt;
  if (rtc_read_datetime(&dt)) {
    if (dt.minute != s_last_min) {
      s_last_min = dt.minute;
      state_changed = true;
    }
  }

  if (state_changed) {
    rook_invalidate_full();
  }

  return 0;
}

static int page_login_on_render(rook_page_t *page, uint32_t *framebuffer,
                                uint32_t stride) {
  (void)page;
  if (!framebuffer)
    return -1;

  uint32_t width = rook_get_width();
  uint32_t height = rook_get_height();
  if (width == 0 || height == 0)
    return -2;

  /* Strict Surface Invariant: RAM canvas is ALWAYS dense (stride == width) */
  uint32_t stride_pixels = width;

  static bool s_logged_metrics = false;
  if (!s_logged_metrics) {
    extern void com1_puts(const char *s);
    com1_puts("[PROBE 3 page_login_on_render] width=");
    char num[16];
    int pos = 0; uint32_t temp = width;
    if (temp == 0) { com1_puts("0"); }
    else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

    com1_puts(" height=");
    pos = 0; temp = height;
    if (temp == 0) { com1_puts("0"); }
    else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

    com1_puts(" raw_stride=");
    pos = 0; temp = stride;
    if (temp == 0) { com1_puts("0"); }
    else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }

    com1_puts(" stride_pixels=");
    pos = 0; temp = stride_pixels;
    if (temp == 0) { com1_puts("0"); }
    else { char t[12]; int ti = 0; while (temp > 0) { t[ti++] = '0' + (temp % 10); temp /= 10; } while (ti > 0) num[pos++] = t[--ti]; num[pos] = '\0'; com1_puts(num); }
    com1_puts("\r\n");

    s_logged_metrics = true;
  }

  int cx = (int)width / 2;
  int cy = (int)height / 2;
  const PointerState *ps = pointer_state_get();

  /* 1. Render Fullscreen Wallpaper */
  wallpaper_service_render(framebuffer, width, height, stride);

  /* Read RTC Date & Time */
  RTCDateTime dt;
  char time_str[16] = "18:54";
  char date_str[32] = "Saturday, Aug 15";

  if (rtc_read_datetime(&dt)) {
    int h = dt.hour % 24;
    int m = dt.minute % 60;
    time_str[0] = '0' + (h / 10);
    time_str[1] = '0' + (h % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (m / 10);
    time_str[4] = '0' + (m % 10);
    time_str[5] = '\0';

    static const char* days[] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
    static const char* months[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    int y = dt.year;
    int mon = dt.month >= 1 && dt.month <= 12 ? dt.month : 8;
    int d = dt.day >= 1 && dt.day <= 31 ? dt.day : 15;
    static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    int y_calc = y - (mon < 3);
    int dow = (y_calc + y_calc/4 - y_calc/100 + y_calc/400 + t[mon-1] + d) % 7;
    if (dow < 0 || dow > 6) dow = 0;

    const char* day_name = days[dow];
    const char* month_name = months[mon-1];
    int pos = 0;
    while (day_name[pos] && pos < 15) { date_str[pos] = day_name[pos]; pos++; }
    date_str[pos++] = ',';
    date_str[pos++] = ' ';
    int mpos = 0;
    while (month_name[mpos] && mpos < 5) { date_str[pos++] = month_name[mpos++]; }
    date_str[pos++] = ' ';
    if (d >= 10) {
      date_str[pos++] = '0' + (d / 10);
      date_str[pos++] = '0' + (d % 10);
    } else {
      date_str[pos++] = '0' + d;
    }
    date_str[pos] = '\0';
  }

  /* 2. State-Based Render Isolation */
  if (s_loading_alpha > 0) {
    draw_desktop_loading_experience(framebuffer, width, height, stride_pixels,
                                    cx, cy, s_loading_alpha, s_loading_elapsed_ms);
  } else if (s_signin_alpha > 0) {
    bool cursor_vis = ((s_cursor_blink_ms / 500) % 2 == 0);
    premium_signin_render(framebuffer, width, height, stride, s_password_buf,
                          s_password_len, s_show_password, cursor_vis,
                          s_password_error, s_signin_alpha);

    /* Bottom-Right Corner Power Controls: Restart (↻) & Shutdown (⏻) (Only on Sign-In / Login screen) */
    int res_btn_x = (int)width - 105;
    int res_btn_y = (int)height - 60;
    int shut_btn_x = (int)width - 55;
    int shut_btn_y = (int)height - 60;

    bool res_hov = (ps && ps->current_x >= res_btn_x && ps->current_x < res_btn_x + 40 &&
                    ps->current_y >= res_btn_y && ps->current_y < res_btn_y + 40);
    bool shut_hov = (ps && ps->current_x >= shut_btn_x && ps->current_x < shut_btn_x + 40 &&
                     ps->current_y >= shut_btn_y && ps->current_y < shut_btn_y + 40);

    /* Restart Container & Icon */
    uint8_t res_fill = (uint8_t)(((res_hov ? 210u : 160u) * s_signin_alpha) / 255u);
    draw_rounded_container(framebuffer, width, height, stride_pixels, res_btn_x + 20,
                           res_btn_y + 20, 40, 20, res_fill);
    draw_atlas_icon_centered(framebuffer, width, height, stride_pixels,
                             g_restart_icon_atlas, PWR_ICON_SIZE, res_btn_x + 20, res_btn_y + 20,
                             s_signin_alpha, false);

    /* Shutdown Container & Icon */
    uint8_t shut_fill = (uint8_t)(((shut_hov ? 210u : 160u) * s_signin_alpha) / 255u);
    draw_rounded_container(framebuffer, width, height, stride_pixels, shut_btn_x + 20,
                           shut_btn_y + 20, 40, 20, shut_fill);
    draw_atlas_icon_centered(framebuffer, width, height, stride_pixels,
                             g_shutdown_icon_atlas, PWR_ICON_SIZE, shut_btn_x + 20, shut_btn_y + 20,
                             s_signin_alpha, false);
  } else if (s_lock_alpha > 0) {
    decode_icons_if_needed();

    /* 1. Top Center Lock Icon (1:1 Razor-Sharp Native Atlas) */
    draw_atlas_icon_centered(framebuffer, width, height, stride_pixels,
                             g_lock_icon_atlas, NATIVE_ICON_SIZE, cx, cy - 245, s_lock_alpha, true);

    /* 2. Apple iOS 17 TrueType Anti-Aliased Date Subtext Header (Above Clock) */
    draw_date_header(framebuffer, width, height, stride_pixels, cx, cy - 195,
                     date_str, s_lock_alpha);

    /* 3. Apple iOS 17 Condensed Tall Frosted Glass Clock (160px height) */
    draw_large_time(framebuffer, width, height, stride_pixels, cx, cy - 150,
                    time_str, s_lock_alpha);

    /* Bottom Center Glass Container Icons: ethernet-port.png (left) and
     * chat.png (right) */
    int container_y = (int)height - 70;
    int box_size = 50;
    int spacing = 16;
    int left_cx = cx - (box_size / 2 + spacing / 2);
    int right_cx = cx + (box_size / 2 + spacing / 2);

    /* Render Glass Containers */
    draw_rounded_container(framebuffer, width, height, stride_pixels, left_cx,
                           container_y, box_size, 10, s_lock_alpha);
    draw_rounded_container(framebuffer, width, height, stride_pixels, right_cx,
                           container_y, box_size, 10, s_lock_alpha);

    /* Render 1:1 Razor-Sharp Native Icons Centered Inside Containers */
    draw_atlas_icon_centered(framebuffer, width, height, stride_pixels,
                             g_ethernet_icon_atlas, NATIVE_ICON_SIZE, left_cx, container_y,
                             s_lock_alpha, false);
    draw_atlas_icon_centered(framebuffer, width, height, stride_pixels,
                             g_chat_icon_atlas, NATIVE_ICON_SIZE, right_cx, container_y,
                             s_lock_alpha, false);
  }

  return 0;
}


static int page_login_on_pause(rook_page_t *page) {
  (void)page;
  return 0;
}
static int page_login_on_resume(rook_page_t *page) {
  (void)page;
  return 0;
}
static int page_login_on_exit(rook_page_t *page) {
  (void)page;
  return 0;
}
static int page_login_on_unload(rook_page_t *page) {
  (void)page;
  return 0;
}
static int page_login_on_destroy(rook_page_t *page) {
  (void)page;
  return 0;
}

rook_page_t *rook_page_login_get(void) {
  if (!s_login_initialized) {
    s_login_page.id = ROOK_PAGE_LOGIN;
    s_login_page.name = "ATOMS Login Page";
    s_login_page.state = ROOK_STATE_UNALLOCATED;

    s_login_page.ops.on_create = page_login_on_create;
    s_login_page.ops.on_init = page_login_on_init;
    s_login_page.ops.on_load = page_login_on_load;
    s_login_page.ops.on_enter = page_login_on_enter;
    s_login_page.ops.on_update = page_login_on_update;
    s_login_page.ops.on_render = page_login_on_render;
    s_login_page.ops.on_pause = page_login_on_pause;
    s_login_page.ops.on_resume = page_login_on_resume;
    s_login_page.ops.on_exit = page_login_on_exit;
    s_login_page.ops.on_unload = page_login_on_unload;
    s_login_page.ops.on_destroy = page_login_on_destroy;
  }
  return &s_login_page;
}
