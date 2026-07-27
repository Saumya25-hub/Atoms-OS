#include "kernel/shell/rook/pages/page_login.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/services/wallpaper/boot_assets.h"
#include "kernel/services/user_profile/user_profile_service.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/ame/include/ame.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/drivers/rtc/rtc.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/core/lib/include/string.h"
#include "bovisual/Include/graphics.h"
#include "bovisual/Include/text.h"
#include "kernel/ui/bofont/bofont.h"

/*
 * ♜ ATOMS OS Login Page (Page 3: ROOK_PAGE_LOGIN)
 * Single Page Architecture: Lock Screen (State 1) -> Sign In (State 2)
 * Driven by ATOMS Motion Engine (AME).
 * 0% Wallpaper reloads, 0% Flicker, 100% Deterministic 60 FPS.
 */

typedef enum {
    LOGIN_STATE_LOCK = 0,
    LOGIN_STATE_TRANSITION,
    LOGIN_STATE_SIGN_IN
} login_page_state_t;

static rook_page_t        s_login_page;
static login_page_state_t s_login_state = LOGIN_STATE_LOCK;
static bool               s_login_initialized = false;

/* Decoded PNG Icon Surfaces */
static struct BOSSurface* s_lock_icon_surf = NULL;
static struct BOSSurface* s_ethernet_icon_surf = NULL;
static struct BOSSurface* s_chat_icon_surf = NULL;
static bool               s_icons_decoded = false;

/* Transition animation telemetry */
static uint64_t s_trans_elapsed_ms = 0;
static uint8_t  s_lock_alpha = 255;
static uint8_t  s_signin_alpha = 0;
static int32_t  s_password_offset_y = 40;

/* Password input buffer */
static char     s_password_buf[64] = {0};
static int      s_password_len = 0;
static uint64_t s_cursor_blink_ms = 0;

static uint32_t blend_alpha(uint32_t bg_color, uint32_t fg_color, uint8_t alpha) {
    if (alpha == 0) return bg_color;
    uint8_t fg_a = (fg_color >> 24) & 0xFF;
    if (fg_a == 0) return bg_color;
    uint32_t eff_alpha = ((uint32_t)fg_a * (uint32_t)alpha) / 255;
    if (eff_alpha == 0) return bg_color;

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
    if (!s_icons_decoded) {
        s_lock_icon_surf     = png_decode(g_boot_ico_lock_png, sizeof(g_boot_ico_lock_png));
        s_ethernet_icon_surf = png_decode(g_boot_ico_ethernet_png, sizeof(g_boot_ico_ethernet_png));
        s_chat_icon_surf     = png_decode(g_boot_ico_chat_png, sizeof(g_boot_ico_chat_png));
        s_icons_decoded = true;
    }
}

/* Render Decoded PNG Surface Tinted as Pure White (Preserving Alpha Mask) */
static void draw_surface_scaled_centered(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, const struct BOSSurface* surf, int cx, int cy, int target_w, int target_h, uint8_t alpha) {
    if (!fb || alpha == 0 || !surf || !surf->framebuffer || surf->width <= 0 || surf->height <= 0) return;

    int src_w = surf->width;
    int src_h = surf->height;
    int start_x = cx - target_w / 2;
    int start_y = cy - target_h / 2;

    for (int y = 0; y < target_h; y++) {
        int py = start_y + y;
        if (py < 0 || py >= (int)fb_h) continue;
        int src_y = (y * src_h) / target_h;
        if (src_y >= src_h) src_y = src_h - 1;

        uint32_t src_line_offset = src_y * src_w;
        uint32_t dst_offset = py * stride_pixels;

        for (int x = 0; x < target_w; x++) {
            int px = start_x + x;
            if (px < 0 || px >= (int)fb_w) continue;
            int src_x = (x * src_w) / target_w;
            if (src_x >= src_w) src_x = src_w - 1;

            uint32_t pixel = surf->framebuffer[src_line_offset + src_x];
            uint8_t icon_alpha = (pixel >> 24) & 0xFF;
            if (icon_alpha > 0) {
                uint8_t eff_a = (uint8_t)(((uint32_t)icon_alpha * (uint32_t)alpha) / 255);
                uint32_t white_pixel = 0x00FFFFFF | ((uint32_t)eff_a << 24);
                fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], white_pixel, eff_a);
            }
        }
    }
}

static uint32_t clock_isqrt(uint32_t n) {
    uint32_t root = 0;
    uint32_t bit = 1U << 30;
    while (bit > n) bit >>= 2;
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

static uint32_t get_digit_dist_sub(int d, int x_sub, int y_sub) {
    uint32_t min_d = 999999;
    uint32_t d_test;

    switch (d) {
        case 0:
            if (y_sub <= 88) d_test = dist_circle_arc(x_sub, y_sub, 88, 88, 72);
            else if (y_sub >= 232) d_test = dist_circle_arc(x_sub, y_sub, 88, 232, 72);
            else {
                uint32_t d_left = dist_sq_seg(x_sub, y_sub, 16, 88, 16, 232);
                uint32_t d_right = dist_sq_seg(x_sub, y_sub, 160, 88, 160, 232);
                d_test = clock_isqrt(d_left < d_right ? d_left : d_right);
            }
            if (d_test < min_d) min_d = d_test;
            break;
        case 1:
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 88, 24, 88, 296));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 56, 72, 88, 24));
            if (d_test < min_d) min_d = d_test;
            break;
        case 2:
            if (y_sub <= 80) d_test = dist_circle_arc(x_sub, y_sub, 88, 80, 64);
            else d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 152, 80, 24, 296));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 24, 296, 152, 296));
            if (d_test < min_d) min_d = d_test;
            break;
        case 3:
            if (y_sub <= 160) d_test = dist_circle_arc(x_sub, y_sub, 88, 88, 64);
            else d_test = dist_circle_arc(x_sub, y_sub, 88, 232, 72);
            if (d_test < min_d) min_d = d_test;
            break;
        case 4:
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 120, 24, 24, 200));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 24, 200, 152, 200));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 120, 24, 120, 296));
            if (d_test < min_d) min_d = d_test;
            break;
        case 5:
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 32, 24, 144, 24));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 32, 24, 32, 136));
            if (d_test < min_d) min_d = d_test;
            d_test = dist_circle_arc(x_sub, y_sub, 80, 216, 80);
            if (d_test < min_d) min_d = d_test;
            break;
        case 6:
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 136, 40, 24, 176));
            if (d_test < min_d) min_d = d_test;
            d_test = dist_circle_arc(x_sub, y_sub, 88, 224, 72);
            if (d_test < min_d) min_d = d_test;
            break;
        case 7:
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 24, 24, 152, 24));
            if (d_test < min_d) min_d = d_test;
            d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 152, 24, 56, 296));
            if (d_test < min_d) min_d = d_test;
            break;
        case 8:
            if (y_sub <= 160) d_test = dist_circle_arc(x_sub, y_sub, 88, 96, 64);
            else d_test = dist_circle_arc(x_sub, y_sub, 88, 224, 72);
            if (d_test < min_d) min_d = d_test;
            break;
        case 9:
            if (y_sub <= 180) d_test = dist_circle_arc(x_sub, y_sub, 88, 96, 72);
            else d_test = clock_isqrt(dist_sq_seg(x_sub, y_sub, 160, 96, 40, 272));
            if (d_test < min_d) min_d = d_test;
            break;
        default:
            break;
    }
    return min_d;
}

/* Razor-Sharp Subpixel Anti-Aliased Rasterizer */
static uint8_t get_subpixel_alpha(int ch, int x, int y, int colon_w) {
    if (ch == ':') {
        int px_sub = x * 4 + 2;
        int py_sub = y * 4 + 2;
        int cx_sub = (colon_w / 2) * 4;
        int d1 = dist_circle_arc(px_sub, py_sub, cx_sub, 112, 0);
        int d2 = dist_circle_arc(px_sub, py_sub, cx_sub, 208, 0);
        int min_d = (d1 < d2) ? d1 : d2;
        if (min_d <= 8) return 255;
        if (min_d <= 12) return (uint8_t)(255 * (12 - min_d) / 4);
        return 0;
    }

    if (ch >= '0' && ch <= '9') {
        int px_sub = x * 4 + 2;
        int py_sub = y * 4 + 2;
        int d_val = ch - '0';
        uint32_t min_d_sub = get_digit_dist_sub(d_val, px_sub, py_sub);

        if (min_d_sub <= 4) return 255;
        if (min_d_sub <= 8) return (uint8_t)(255 * (8 - min_d_sub) / 4);
    }

    return 0;
}

/* Render Modern Ultra-Thin Razor-Sharp Lock Screen Clock */
static void draw_large_time(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int cy, const char* time_str, uint8_t alpha) {
    if (!fb || !time_str || alpha == 0) return;

    int len = 0;
    while (time_str[len]) len++;

    int digit_w = 44;
    int digit_h = 80;
    int spacing = 12;
    int colon_w = 20;

    int total_w = 0;
    for (int i = 0; i < len; i++) {
        total_w += (time_str[i] == ':') ? colon_w : digit_w;
        if (i < len - 1) total_w += spacing;
    }

    int start_x = cx - total_w / 2;
    int curr_x = start_x;

    for (int i = 0; i < len; i++) {
        char ch = time_str[i];
        int w = (ch == ':') ? colon_w : digit_w;

        for (int y = 0; y < digit_h; y++) {
            int py = cy + y;
            if (py < 0 || py >= (int)fb_h) continue;
            uint32_t dst_offset = py * stride_pixels;

            for (int x = 0; x < w; x++) {
                int px = curr_x + x;
                if (px < 0 || px >= (int)fb_w) continue;

                uint8_t sub_alpha = get_subpixel_alpha(ch, x, y, colon_w);

                if (sub_alpha > 0) {
                    uint8_t final_alpha = (uint8_t)(((uint32_t)sub_alpha * (uint32_t)alpha) / 255);
                    fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)final_alpha << 24), final_alpha);
                }
            }
        }

        curr_x += w + spacing;
    }
}

/* Render Modern Glass Rounded Container for Bottom Icons */
static void draw_rounded_container(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int cy, int size, int radius, uint8_t alpha) {
    if (!fb || alpha == 0) return;

    int half = size / 2;
    int x1 = cx - half;
    int y1 = cy - half;
    int x2 = cx + half;
    int y2 = cy + half;

    for (int py = y1; py <= y2; py++) {
        if (py < 0 || py >= (int)fb_h) continue;
        uint32_t dst_offset = py * stride_pixels;

        for (int px = x1; px <= x2; px++) {
            if (px < 0 || px >= (int)fb_w) continue;

            int dx = (px < x1 + radius) ? (x1 + radius - px) : ((px > x2 - radius) ? (px - (x2 - radius)) : 0);
            int dy = (py < y1 + radius) ? (y1 + radius - py) : ((py > y2 - radius) ? (py - (y2 - radius)) : 0);
            int d_corner = (dx > 0 && dy > 0) ? (int)clock_isqrt(dx * dx + dy * dy) : 0;

            if (d_corner > radius) continue;

            // Translucent glass fill
            uint8_t bg_a = (uint8_t)((35 * alpha) / 255);
            fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)bg_a << 24), bg_a);

            // Thin crisp white border outline
            bool is_border = false;
            if (dx > 0 && dy > 0) {
                if (d_corner >= radius - 2 && d_corner <= radius) is_border = true;
            } else {
                if (px == x1 || px == x2 || py == y1 || py == y2) is_border = true;
            }

            if (is_border) {
                uint8_t border_a = (uint8_t)((180 * alpha) / 255);
                fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0x00FFFFFF | ((uint32_t)border_a << 24), border_a);
            }
        }
    }
}

/* Vector/BOFont character renderer for Date & Text */
static void draw_custom_text(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int y, const char* str, uint32_t color, bool large, uint8_t alpha) {
    if (!str || alpha == 0) return;

    int len = 0;
    while (str[len]) len++;

    int char_w = large ? 14 : 9;
    int char_h = large ? 24 : 15;
    int spacing = 2;

    int total_w = len * char_w + (len - 1) * spacing;
    int start_x = cx - total_w / 2;

    for (int i = 0; i < len; i++) {
        char ch = str[i];
        int char_x = start_x + i * (char_w + spacing);

        if (ch == ' ') continue;

        for (int cy_pos = 0; cy_pos < char_h; cy_pos++) {
            int py = y + cy_pos;
            if (py < 0 || py >= (int)fb_h) continue;
            uint32_t dst_offset = py * stride_pixels;

            for (int cx_pos = 0; cx_pos < char_w; cx_pos++) {
                int px = char_x + cx_pos;
                if (px < 0 || px >= (int)fb_w) continue;

                bool stroke = false;
                int norm_x = cx_pos * 8 / char_w;
                int norm_y = cy_pos * 8 / char_h;

                if (ch >= 'A' && ch <= 'Z') {
                    if (norm_x == 0 || norm_x == 7 || norm_y == 0 || norm_y == 4 || norm_y == 7) stroke = true;
                } else if (ch >= 'a' && ch <= 'z') {
                    if (norm_x == 0 || norm_x == 7 || norm_y == 3 || norm_y == 7) stroke = true;
                } else if (ch >= '0' && ch <= '9') {
                    if (norm_x == 0 || norm_x == 7 || norm_y == 0 || norm_y == 7) stroke = true;
                } else if (ch == ',') {
                    if (norm_x >= 3 && norm_x <= 5 && norm_y >= 6 && norm_y <= 7) stroke = true;
                }

                if (stroke) {
                    uint32_t eff_color = color | ((uint32_t)alpha << 24);
                    fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], eff_color, alpha);
                }
            }
        }
    }
}

/* Render Password Entry Input Box */
static void draw_password_box(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int cy, int password_len, bool cursor_visible, uint8_t alpha) {
    if (!fb || alpha == 0) return;

    int box_w = 260;
    int box_h = 44;
    int start_x = cx - box_w / 2;
    int start_y = cy - box_h / 2;

    for (int y = 0; y < box_h; y++) {
        int py = start_y + y;
        if (py < 0 || py >= (int)fb_h) continue;
        uint32_t dst_offset = py * stride_pixels;
        for (int x = 0; x < box_w; x++) {
            int px = start_x + x;
            if (px < 0 || px >= (int)fb_w) continue;

            bool is_border = (x == 0 || x == box_w - 1 || y == 0 || y == box_h - 1);
            uint32_t fg = is_border ? 0x0094A3B8 : 0x000F172A;
            uint8_t eff_alpha = is_border ? (uint8_t)((200 * alpha) / 255) : (uint8_t)((160 * alpha) / 255);
            uint32_t fg_with_a = fg | ((uint32_t)eff_alpha << 24);

            fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], fg_with_a, eff_alpha);
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
            if (py < 0 || py >= (int)fb_h) continue;
            uint32_t dst_offset = py * stride_pixels;
            for (int dx = -3; dx <= 3; dx++) {
                int px = dot_cx + dx;
                if (px < 0 || px >= (int)fb_w) continue;
                if (dx * dx + dy * dy <= 3 * 3) {
                    fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, alpha);
                }
            }
        }
    }

    if (cursor_visible) {
        int cursor_x = (password_len > 0) ? (dots_start_x + password_len * dot_spacing + 4) : cx;
        int cursor_y_top = cy - 10;
        int cursor_y_bot = cy + 10;

        for (int py = cursor_y_top; py <= cursor_y_bot; py++) {
            if (py < 0 || py >= (int)fb_h) continue;
            uint32_t dst_offset = py * stride_pixels;
            for (int px = cursor_x - 1; px <= cursor_x + 1; px++) {
                if (px < 0 || px >= (int)fb_w) continue;
                fb[dst_offset + px] = blend_alpha(fb[dst_offset + px], 0xFFFFFFFF, alpha);
            }
        }
    }
}

static int page_login_on_create(rook_page_t* page) {
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
    s_login_initialized = true;
    return 0;
}

static int page_login_on_init(rook_page_t* page) {
    (void)page;
    s_login_state = LOGIN_STATE_LOCK;
    s_lock_alpha = 255;
    s_signin_alpha = 0;
    s_password_offset_y = 40;
    s_trans_elapsed_ms = 0;
    s_password_len = 0;
    return 0;
}

static int page_login_on_load(rook_page_t* page) {
    (void)page;
    return 0;
}

static int page_login_on_enter(rook_page_t* page) {
    (void)page;
    wallpaper_service_init();
    decode_icons_if_needed();
    s_login_state = LOGIN_STATE_LOCK;
    s_lock_alpha = 255;
    s_signin_alpha = 0;
    s_password_offset_y = 40;
    s_trans_elapsed_ms = 0;
    return 0;
}

static int page_login_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    s_cursor_blink_ms += delta_ms;

    KeyboardEvent key_evt;
    bool key_pressed = keyboard_poll_event(&key_evt);

    if (s_login_state == LOGIN_STATE_LOCK) {
        if (key_pressed && key_evt.pressed) {
            s_login_state = LOGIN_STATE_TRANSITION;
            s_trans_elapsed_ms = 0;
        }
    } else if (s_login_state == LOGIN_STATE_TRANSITION) {
        s_trans_elapsed_ms += delta_ms;

        float p = (float)s_trans_elapsed_ms / 400.0f;
        if (p > 1.0f) p = 1.0f;
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
        if (key_pressed && key_evt.pressed) {
            if (key_evt.keycode == 0x1C || key_evt.ascii == '\n' || key_evt.ascii == '\r') {
                rook_goto(ROOK_PAGE_DESKTOP);
            } else if (key_evt.keycode == 0x0E || key_evt.ascii == '\b') {
                if (s_password_len > 0) {
                    s_password_len--;
                    s_password_buf[s_password_len] = '\0';
                }
            } else if (key_evt.ascii >= 32 && key_evt.ascii <= 126) {
                if (s_password_len < 63) {
                    s_password_buf[s_password_len++] = key_evt.ascii;
                    s_password_buf[s_password_len] = '\0';
                }
            }
        }
    }

    return 0;
}

static int page_login_on_render(rook_page_t* page, uint32_t* framebuffer, uint32_t stride) {
    (void)page;
    if (!framebuffer) return -1;

    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0 || height == 0) return -2;

    uint32_t stride_pixels = stride / 4;
    if (stride_pixels == 0) stride_pixels = width;

    int cx = (int)width / 2;
    int cy = (int)height / 2;

    /* 1. Render Fullscreen Wallpaper */
    wallpaper_service_render(framebuffer, width, height, stride);

    /* Read RTC Date & Time */
    RTCDateTime dt;
    char time_str[16] = "10:12";
    char date_str[32] = "Saturday, October 11";

    if (rtc_read_datetime(&dt)) {
        int h = dt.hour % 24;
        int m = dt.minute % 60;
        time_str[0] = '0' + (h / 10);
        time_str[1] = '0' + (h % 10);
        time_str[2] = ':';
        time_str[3] = '0' + (m / 10);
        time_str[4] = '0' + (m % 10);
        time_str[5] = '\0';
    }

    /* 2. State 1: Lock Screen UI */
    if (s_lock_alpha > 0) {
        decode_icons_if_needed();

        /* Top Center Lock Icon (lock.png, 26x26px) */
        draw_surface_scaled_centered(framebuffer, width, height, stride_pixels, s_lock_icon_surf, cx, cy - 140, 26, 26, s_lock_alpha);

        /* Large Ultra-Thin Clock */
        draw_large_time(framebuffer, width, height, stride_pixels, cx, cy - 100, time_str, s_lock_alpha);

        /* Date Subtext */
        draw_custom_text(framebuffer, width, height, stride_pixels, cx, cy - 15, date_str, 0x00F8FAFC, false, s_lock_alpha);

        /* Bottom Center Glass Container Icons: ethernet-port.png (left) and chat.png (right) */
        int container_y = (int)height - 70;
        int box_size = 50;
        int icon_size = 24;
        int spacing = 16;
        int left_cx = cx - (box_size / 2 + spacing / 2);
        int right_cx = cx + (box_size / 2 + spacing / 2);

        /* Render Glass Containers */
        draw_rounded_container(framebuffer, width, height, stride_pixels, left_cx, container_y, box_size, 10, s_lock_alpha);
        draw_rounded_container(framebuffer, width, height, stride_pixels, right_cx, container_y, box_size, 10, s_lock_alpha);

        /* Render Pure White PNG Icons Centered Inside Containers */
        draw_surface_scaled_centered(framebuffer, width, height, stride_pixels, s_ethernet_icon_surf, left_cx, container_y, icon_size, icon_size, s_lock_alpha);
        draw_surface_scaled_centered(framebuffer, width, height, stride_pixels, s_chat_icon_surf, right_cx, container_y, icon_size, icon_size, s_lock_alpha);
    }

    /* 3. State 2: Sign In UI */
    if (s_signin_alpha > 0) {
        int avatar_y = cy - 80 + s_password_offset_y;
        int welcome_y = cy + 5 + s_password_offset_y;
        int passbox_y = cy + 65 + s_password_offset_y;

        user_profile_service_render_avatar(framebuffer, width, height, stride_pixels, cx, avatar_y, 48, s_signin_alpha);
        draw_custom_text(framebuffer, width, height, stride_pixels, cx, welcome_y, "Welcome, Saumya", 0x00FFFFFF, true, s_signin_alpha);

        bool cursor_vis = ((s_cursor_blink_ms / 500) % 2 == 0);
        draw_password_box(framebuffer, width, height, stride_pixels, cx, passbox_y, s_password_len, cursor_vis, s_signin_alpha);
    }

    rook_invalidate_full();
    return 0;
}

static int page_login_on_pause(rook_page_t* page) { (void)page; return 0; }
static int page_login_on_resume(rook_page_t* page) { (void)page; return 0; }
static int page_login_on_exit(rook_page_t* page) { (void)page; return 0; }
static int page_login_on_unload(rook_page_t* page) { (void)page; return 0; }
static int page_login_on_destroy(rook_page_t* page) { (void)page; return 0; }

rook_page_t* rook_page_login_get(void) {
    if (!s_login_initialized) {
        s_login_page.id = ROOK_PAGE_LOGIN;
        s_login_page.name = "ATOMS Login Page";
        s_login_page.state = ROOK_STATE_UNALLOCATED;

        s_login_page.ops.on_create  = page_login_on_create;
        s_login_page.ops.on_init    = page_login_on_init;
        s_login_page.ops.on_load    = page_login_on_load;
        s_login_page.ops.on_enter   = page_login_on_enter;
        s_login_page.ops.on_update  = page_login_on_update;
        s_login_page.ops.on_render  = page_login_on_render;
        s_login_page.ops.on_pause   = page_login_on_pause;
        s_login_page.ops.on_resume  = page_login_on_resume;
        s_login_page.ops.on_exit    = page_login_on_exit;
        s_login_page.ops.on_unload  = page_login_on_unload;
        s_login_page.ops.on_destroy = page_login_on_destroy;
    }
    return &s_login_page;
}
