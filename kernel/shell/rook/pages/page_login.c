#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/identity/include/identity.h"
#include "kernel/ame/include/ame.h"
#include "bovisual/Include/events.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/ui/bofont/bofont.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/debug/step14_telemetry.h"

/*
 * ♜ ROOK ENGINE V1.0 — Page 3: ATOMS OS Login Screen
 * High-Fidelity Hybrid Login Screen: DWM Cached Rendering, Dirty Rectangles & Rich Interactive Polish
 */

/* State variables */
static uint32_t g_login_ticks = 0;
static char s_password_buf[64] = "";
static int s_password_len = 0;
static bool s_error_state = false;
static AME_Handle s_shake_handle = AME_INVALID_HANDLE;

/* DWM Cached Rendering state */
static uint32_t* s_login_cache_buffer = NULL;
static bool s_cache_valid = false;

/* Interactive UX state */
static bool s_show_password = false;
static uint32_t s_caret_timer = 0;
static bool s_caret_visible = true;
static bool s_input_focused = true;

/* Button Micro-Animations state */
static bool s_btn_hovered = false;
static bool s_btn_pressed = false;
static AME_Handle s_btn_hover_handle = AME_INVALID_HANDLE;
static AME_Handle s_btn_press_handle = AME_INVALID_HANDLE;

/* Error state & timer */
static uint32_t s_error_timer = 0;
static AME_Handle s_error_fade_handle = AME_INVALID_HANDLE;
static int32_t s_error_opacity = 0;

/* Success loading state */
static bool s_auth_success_loading = false;
static uint32_t s_loading_timer = 0;

static uint32_t* g_wallpaper_buffer = NULL;
static int g_wallpaper_fd = -1;
static uint32_t g_wallpaper_bytes_loaded = 0;
static bool g_wallpaper_loaded = false;

uint32_t* rook_get_wallpaper_buffer(void) {
    return g_wallpaper_buffer;
}

bool rook_is_wallpaper_loaded(void) {
    return g_wallpaper_loaded;
}

/* Utility pixel drawer */
static inline void login_putpixel(uint32_t* fb, uint32_t w, uint32_t h, int32_t x, int32_t y, uint32_t color) {
    if (x >= 0 && x < (int32_t)w && y >= 0 && y < (int32_t)h) {
        fb[y * w + x] = color;
    }
}

/* Integer Square Root helper for fast distance math */
static uint32_t fast_sqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = 1;
    while (x > y) {
        x = (x + y) >> 1;
        y = n / x;
    }
    return x;
}

/* 2D Pseudo-random noise generator for procedural galaxy dust */
static inline uint32_t noise_2d(int32_t x, int32_t y) {
    uint32_t n = (uint32_t)(x * 374761393 + y * 668265263);
    n = (n ^ (n >> 13)) * 1274126177;
    return n ^ (n >> 16);
}

/* Draw a minimalist character (8x8 grid scaled up) */
static void login_draw_char(uint32_t* fb, int fb_w, int fb_h, char c, int x, int y, int scale, uint32_t color) {
    static const uint8_t font_data[128][8] = {
        ['A'] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00},
        ['B'] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
        ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
        ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
        ['E'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
        ['F'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00},
        ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
        ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
        ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
        ['J'] = {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00},
        ['K'] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00},
        ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00},
        ['M'] = {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00},
        ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
        ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
        ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
        ['Q'] = {0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00},
        ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00},
        ['S'] = {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00},
        ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
        ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
        ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
        ['W'] = {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},
        ['X'] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00},
        ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
        ['Z'] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00},
        ['0'] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00},
        ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
        ['2'] = {0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E, 0x00},
        ['3'] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
        ['4'] = {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00},
        ['5'] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00},
        ['6'] = {0x3C, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x3C, 0x00},
        ['7'] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
        ['8'] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00},
        ['9'] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00},
        [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
        ['*'] = {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},
        ['-'] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
        ['>'] = {0x00, 0x18, 0x30, 0x60, 0x30, 0x18, 0x00, 0x00},
        ['a'] = {0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00},
        ['b'] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x00},
        ['c'] = {0x00, 0x00, 0x3C, 0x60, 0x60, 0x60, 0x3C, 0x00},
        ['d'] = {0x06, 0x06, 0x3E, 0x66, 0x66, 0x66, 0x3E, 0x00},
        ['e'] = {0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00},
        ['f'] = {0x1C, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00},
        ['g'] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C},
        ['h'] = {0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00},
        ['i'] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3C, 0x00},
        ['j'] = {0x06, 0x00, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C},
        ['k'] = {0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0x00},
        ['l'] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
        ['m'] = {0x00, 0x00, 0x66, 0x7F, 0x6B, 0x63, 0x63, 0x00},
        ['n'] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00},
        ['o'] = {0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00},
        ['p'] = {0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60},
        ['q'] = {0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06},
        ['r'] = {0x00, 0x00, 0x5C, 0x66, 0x60, 0x60, 0x60, 0x00},
        ['s'] = {0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00},
        ['t'] = {0x30, 0x30, 0x78, 0x30, 0x30, 0x30, 0x1C, 0x00},
        ['u'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00},
        ['v'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
        ['w'] = {0x00, 0x00, 0x63, 0x63, 0x6B, 0x7F, 0x36, 0x00},
        ['x'] = {0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00},
        ['y'] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x3C},
        ['z'] = {0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00}
    };

    uint8_t idx = (uint8_t)c;
    if (idx > 127) idx = '?';

    for (int row = 0; row < 8; row++) {
        uint8_t bits = font_data[idx][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        login_putpixel(fb, fb_w, fb_h, x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

static void login_draw_atom_logo(uint32_t* fb, uint32_t width, uint32_t height, int center_x, int center_y, int radius, int nucleus_radius, int dot_radius, int tilt_offset) {
    login_draw_char(fb, width, height, '.', center_x - 3, center_y - 8, 1, 0xFFFFFFFF);
    login_draw_char(fb, width, height, '.', center_x + 8, center_y + 2, 1, 0xFFFFFFFF);
    login_draw_char(fb, width, height, '.', center_x - 10, center_y + 4, 1, 0xFFFFFFFF);
    login_draw_char(fb, width, height, '.', center_x + 4, center_y - 12, 1, 0xFFFFFFFF);

    for (int dy = -nucleus_radius; dy <= nucleus_radius; dy++) {
        for (int dx = -nucleus_radius; dx <= nucleus_radius; dx++) {
            int d2 = dx*dx + dy*dy;
            if (d2 <= nucleus_radius * nucleus_radius) {
                uint32_t color = 0xFFFFFFFF;
                if (d2 > (nucleus_radius - 2) * (nucleus_radius - 2)) {
                    color = 0xFFCCCCCC;
                } else if (d2 > (nucleus_radius - 4) * (nucleus_radius - 4)) {
                    color = 0xFFEEEEEE;
                }
                login_putpixel(fb, width, height, center_x + dx, center_y + dy, color);
            }
        }
    }

    int num_pts = 160;
    for (int i = 0; i < num_pts; i++) {
        int x = (i * (2 * radius)) / num_pts - radius;
        int y = (x * x) / (2 * radius) - (radius / 2);

        int sx1 = center_x + x;
        int sy1 = center_y + y - tilt_offset;
        login_putpixel(fb, width, height, sx1, sy1, 0xFFFFFFFF);
        login_putpixel(fb, width, height, sx1, sy1 + 1, 0xFF888888);
        login_putpixel(fb, width, height, sx1 + 1, sy1, 0xFF888888);

        int sx2 = center_x + x;
        int sy2 = center_y - y + tilt_offset;
        login_putpixel(fb, width, height, sx2, sy2, 0xFFFFFFFF);
        login_putpixel(fb, width, height, sx2, sy2 + 1, 0xFF888888);
        login_putpixel(fb, width, height, sx2 + 1, sy2, 0xFF888888);

        int sx3 = center_x + y - tilt_offset;
        int sy3 = center_y + x;
        login_putpixel(fb, width, height, sx3, sy3, 0xFFFFFFFF);
        login_putpixel(fb, width, height, sx3 + 1, sy3, 0xFF888888);
        login_putpixel(fb, width, height, sx3, sy3 + 1, 0xFF888888);
    }

    int e1_x = center_x + (radius * 3) / 4;
    int e1_y = center_y + ((radius * 3) / 4 * (radius * 3) / 4) / (2 * radius) - (radius / 2) - tilt_offset;
    for (int dy = -dot_radius; dy <= dot_radius; dy++) {
        for (int dx = -dot_radius; dx <= dot_radius; dx++) {
            if (dx*dx + dy*dy <= dot_radius*dot_radius) {
                login_putpixel(fb, width, height, e1_x + dx, e1_y + dy, 0xFF00DDFF);
            }
        }
    }

    int e2_x = center_x - (radius * 3) / 4;
    int e2_y = center_y - ((radius * 3) / 4 * (radius * 3) / 4) / (2 * radius) + (radius / 2) + tilt_offset;
    for (int dy = -dot_radius; dy <= dot_radius; dy++) {
        for (int dx = -dot_radius; dx <= dot_radius; dx++) {
            if (dx*dx + dy*dy <= dot_radius*dot_radius) {
                login_putpixel(fb, width, height, e2_x + dx, e2_y + dy, 0xFFFF8800);
            }
        }
    }

    int e3_x = center_x + ((radius * 3) / 4 * (radius * 3) / 4) / (2 * radius) - (radius / 2) - tilt_offset;
    int e3_y = center_y + (radius * 3) / 4;
    for (int dy = -dot_radius; dy <= dot_radius; dy++) {
        for (int dx = -dot_radius; dx <= dot_radius; dx++) {
            if (dx*dx + dy*dy <= dot_radius*dot_radius) {
                login_putpixel(fb, width, height, e3_x + dx, e3_y + dy, 0xFF00FF88);
            }
        }
    }
}

static void draw_centered_str(uint32_t* fb, int fb_w, int fb_h, const char* str, int y, uint32_t color, int scale, int char_space, int x_offset) {
    (void)fb;
    (void)fb_h;
    (void)char_space;
    BOFontRole role = BOFONT_ROLE_UI_MEDIUM;
    if (scale >= 3) role = BOFONT_ROLE_TITLE;
    else if (scale == 1) role = BOFONT_ROLE_CAPTION;

    BOTextMetrics tm = BOFont_MeasureTextRole(role, str);
    int start_x = (fb_w - tm.width) / 2 + x_offset;
    if (start_x < 0) start_x = 0;
    BOFont_DrawTextRole(role, str, start_x, y, color);
}

static void draw_box(uint32_t* fb, int fb_w, int fb_h, int x, int y, int w, int h, uint32_t border_color, uint32_t fill_color) {
    for (int py = y; py < y + h; py++) {
        if (py < 0 || py >= fb_h) continue;
        for (int px = x; px < x + w; px++) {
            if (px < 0 || px >= fb_w) continue;
            if (py == y || py == y + h - 1 || px == x || px == x + w - 1) {
                fb[py * fb_w + px] = border_color;
            } else if (fill_color != 0) {
                fb[py * fb_w + px] = fill_color;
            }
        }
    }
}

static void login_fill_circle(uint32_t* fb, uint32_t w, uint32_t h, int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    for (int32_t dy = -r; dy <= r; dy++) {
        for (int32_t dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                login_putpixel(fb, w, h, cx + dx, cy + dy, color);
            }
        }
    }
}

static void draw_visible_spiral_galaxy(uint32_t* fb, uint32_t width, uint32_t height) {
    int32_t cx = (int32_t)width / 2;
    int32_t cy = (int32_t)height / 2;

    for (uint32_t y = 0; y < height; y++) {
        uint32_t row_offset = y * width;
        int32_t dy = (int32_t)y - cy;
        
        for (uint32_t x = 0; x < width; x++) {
            int32_t dx = (int32_t)x - cx;
            
            int32_t rot_x = (dx * 883 + dy * 469) / 1000;
            int32_t rot_y = (-dx * 469 + dy * 883) / 1000;
            
            uint32_t ell_dist = fast_sqrt((uint32_t)(rot_x * rot_x + (rot_y * rot_y * 100 / 36)));
            
            uint8_t intensity = 0;
            if (ell_dist < 760) {
                uint32_t core_val = 0;
                if (ell_dist < 260) {
                    core_val = ((260 - ell_dist) * 255) / 260;
                    core_val = (core_val * core_val) / 255;
                }

                uint32_t angle_deg = 0;
                if (rot_x != 0 || rot_y != 0) {
                    int32_t abs_x = rot_x > 0 ? rot_x : -rot_x;
                    int32_t abs_y = rot_y > 0 ? rot_y : -rot_y;
                    if (abs_x >= abs_y) {
                        angle_deg = (uint32_t)(abs_y * 45 / abs_x);
                        if (rot_x < 0 && rot_y >= 0) angle_deg = 180 - angle_deg;
                        else if (rot_x < 0 && rot_y < 0) angle_deg = 180 + angle_deg;
                        else if (rot_x >= 0 && rot_y < 0) angle_deg = 360 - angle_deg;
                    } else {
                        angle_deg = 90 - (uint32_t)(abs_x * 45 / abs_y);
                        if (rot_x < 0 && rot_y >= 0) angle_deg = 180 - angle_deg;
                        else if (rot_x < 0 && rot_y < 0) angle_deg = 180 + angle_deg;
                        else if (rot_x >= 0 && rot_y < 0) angle_deg = 360 - angle_deg;
                    }
                }

                uint32_t spiral_shift = (ell_dist * 130) / 100;
                uint32_t arm_angle = (angle_deg + spiral_shift) % 180;
                
                uint32_t dist_to_arm = arm_angle > 90 ? (180 - arm_angle) : arm_angle;
                
                uint32_t arm_val = 0;
                if (ell_dist > 60 && ell_dist < 760) {
                    uint32_t base_arm = (90 - dist_to_arm) * (90 - dist_to_arm) / 80;
                    uint32_t radial_fade = ((760 - ell_dist) * 256) / 700;
                    arm_val = (base_arm * radial_fade) / 256;
                }

                uint32_t combined = core_val + arm_val;
                uint32_t n = noise_2d((int32_t)x, (int32_t)y) % 24;
                if (combined > 4) {
                    combined = combined - 4 + (n * combined / 90);
                }

                intensity = (uint8_t)(combined <= 255 ? combined : 255);
            }

            uint8_t r = intensity, g = intensity, b = intensity;
            if (ell_dist < 260) {
                uint32_t t = 260 - ell_dist;
                r = (intensity * (100 + (35 * t / 260))) / 100;
                g = (intensity * (100 + (5 * t / 260))) / 100;
                b = (intensity * (100 - (35 * t / 260))) / 100;
            }

            fb[row_offset + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    uint32_t seed = 9999;
    for (uint32_t i = 0; i < 1700; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t sx = seed % width;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t sy = seed % height;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t mag = seed % 16;

        uint32_t star_color = 0xFF282828;
        if (mag == 0) star_color = 0xFFFFFFFF;
        else if (mag < 3) star_color = 0xFFCCCCCC;
        else if (mag < 8) star_color = 0xFF666666;

        login_putpixel(fb, width, height, sx, sy, star_color);

        if (mag == 0) {
            login_putpixel(fb, width, height, sx - 1, sy, 0xFF999999);
            login_putpixel(fb, width, height, sx + 1, sy, 0xFF999999);
            login_putpixel(fb, width, height, sx, sy - 1, 0xFF999999);
            login_putpixel(fb, width, height, sx, sy + 1, 0xFF999999);
        }
    }
}

static void login_attempt_auth(void) {
    if (s_auth_success_loading) return;
    
    extern void Desktop_Shell_StartBootExperience(void);
    uint32_t width = rook_get_width();
    if (width == 0) width = 1024;
    int center_x = width / 2;
    int card_y = 205;
    BWE_Rect card_rect = { center_x - 230, card_y, 460, 320 };
    
    bool success = Identity_Authenticate("admin", s_password_buf);
    if (success) {
        s_error_state = false;
        s_error_opacity = 0;
        s_auth_success_loading = true;
        s_loading_timer = 600; // 600ms loading delay
        BWE_AddCompositorDirtyRect(&card_rect);
    } else {
        s_error_state = true;
        s_password_len = 0;
        s_password_buf[0] = '\0';
        s_error_timer = 2000;
        s_error_opacity = 0;
        
        if (s_shake_handle != AME_INVALID_HANDLE) {
            AME_DestroyAnimation(s_shake_handle);
        }
        s_shake_handle = AME_CreateAnimation(NULL, PROP_X, 24, 0, 500, EASE_OUT_ELASTIC);
        AME_Play(s_shake_handle);
        
        if (s_error_fade_handle != AME_INVALID_HANDLE) {
            AME_DestroyAnimation(s_error_fade_handle);
        }
        s_error_fade_handle = AME_CreateAnimation(NULL, PROP_OPACITY, 0, 255, 300, EASE_OUT_CUBIC);
        AME_Play(s_error_fade_handle);
        
        BWE_AddCompositorDirtyRect(&card_rect);
    }
}

void page_login_handle_event(const BVEvent* ev) {
    if (!ev || s_auth_success_loading) return;
    extern void display_print(const char*);
    display_print("[INPUT TRACE] page_login_handle_event\n");

    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0) width = 1024;
    if (height == 0) height = 768;
    int center_x = width / 2;
    int card_y = 205;

    int input_w = 300;
    int input_h = 42;
    int input_x = center_x - input_w / 2;
    int input_y = card_y + 170;
    BWE_Rect input_box_rect = { input_x - 5, input_y - 5, input_w + 10, input_h + 10 };

    int btn_x = center_x - 80;
    int btn_y = card_y + 250;
    int btn_w = 160;
    int btn_h = 36;
    BWE_Rect btn_rect = { btn_x - 5, btn_y - 5, btn_w + 10, btn_h + 10 };

    if (ev->type == BV_EVENT_KEY_DOWN) {
        if (s_error_state) {
            s_error_state = false;
            s_error_opacity = 0;
            BWE_AddCompositorDirtyRect(&input_box_rect);
        }

        // Backspace
        if (ev->key_code == 0x0E || ev->ascii == 8 || ev->ascii == '\b' || ev->key_code == 0x08) {
            if (s_password_len > 0) {
                s_password_len--;
                s_password_buf[s_password_len] = '\0';
                s_caret_visible = true;
                s_caret_timer = 0;
                BWE_AddCompositorDirtyRect(&input_box_rect);
            }
            return;
        }

        // Enter key -> Trigger login
        if (ev->key_code == 0x1C || ev->ascii == '\r' || ev->ascii == '\n') {
            login_attempt_auth();
            return;
        }

        // Printable ASCII
        if (ev->ascii >= 32 && ev->ascii <= 126) {
            if (s_password_len < (int)sizeof(s_password_buf) - 1) {
                s_password_buf[s_password_len] = ev->ascii;
                s_password_len++;
                s_password_buf[s_password_len] = '\0';
                s_caret_visible = true;
                s_caret_timer = 0;
                BWE_AddCompositorDirtyRect(&input_box_rect);
            }
        }
    } else if (ev->type == BV_EVENT_MOUSE_MOVE) {
        bool inside_btn = (ev->mouse_x >= btn_x && ev->mouse_x <= btn_x + btn_w &&
                           ev->mouse_y >= btn_y && ev->mouse_y <= btn_y + btn_h);
        if (inside_btn && !s_btn_hovered) {
            s_btn_hovered = true;
            if (s_btn_hover_handle != AME_INVALID_HANDLE) AME_DestroyAnimation(s_btn_hover_handle);
            s_btn_hover_handle = AME_CreateAnimation(NULL, PROP_OPACITY, 128, 255, 250, EASE_OUT_CUBIC);
            AME_Play(s_btn_hover_handle);
            BWE_AddCompositorDirtyRect(&btn_rect);
        } else if (!inside_btn && s_btn_hovered) {
            s_btn_hovered = false;
            if (s_btn_hover_handle != AME_INVALID_HANDLE) AME_DestroyAnimation(s_btn_hover_handle);
            s_btn_hover_handle = AME_CreateAnimation(NULL, PROP_OPACITY, 255, 128, 250, EASE_OUT_CUBIC);
            AME_Play(s_btn_hover_handle);
            BWE_AddCompositorDirtyRect(&btn_rect);
        }
    } else if (ev->type == BV_EVENT_MOUSE_DOWN) {
        // Check Eye icon click (right side of input box: x from input_x + input_w - 38 to input_x + input_w - 10)
        if (ev->mouse_x >= input_x + input_w - 38 && ev->mouse_x <= input_x + input_w - 10 &&
            ev->mouse_y >= input_y + 8 && ev->mouse_y <= input_y + 34) {
            s_show_password = !s_show_password;
            BWE_AddCompositorDirtyRect(&input_box_rect);
            return;
        }

        // Check Input box click
        if (ev->mouse_x >= input_x && ev->mouse_x <= input_x + input_w &&
            ev->mouse_y >= input_y && ev->mouse_y <= input_y + input_h) {
            s_input_focused = true;
            s_caret_visible = true;
            s_caret_timer = 0;
            BWE_AddCompositorDirtyRect(&input_box_rect);
            return;
        }

        // Check LOGIN button click
        if (ev->mouse_x >= btn_x && ev->mouse_x <= btn_x + btn_w &&
            ev->mouse_y >= btn_y && ev->mouse_y <= btn_y + btn_h) {
            s_btn_pressed = true;
            if (s_btn_press_handle != AME_INVALID_HANDLE) AME_DestroyAnimation(s_btn_press_handle);
            s_btn_press_handle = AME_CreateAnimation(NULL, PROP_Y, 0, 2, 100, EASE_OUT_CUBIC);
            AME_Play(s_btn_press_handle);
            BWE_AddCompositorDirtyRect(&btn_rect);
        }
    } else if (ev->type == BV_EVENT_MOUSE_UP) {
        if (s_btn_pressed) {
            s_btn_pressed = false;
            if (s_btn_press_handle != AME_INVALID_HANDLE) AME_DestroyAnimation(s_btn_press_handle);
            s_btn_press_handle = AME_CreateAnimation(NULL, PROP_Y, 2, 0, 200, EASE_OUT_CUBIC);
            AME_Play(s_btn_press_handle);
            BWE_AddCompositorDirtyRect(&btn_rect);

            if (ev->mouse_x >= btn_x && ev->mouse_x <= btn_x + btn_w &&
                ev->mouse_y >= btn_y && ev->mouse_y <= btn_y + btn_h) {
                login_attempt_auth();
            }
        }
    }
}

static int login_on_create(rook_page_t* page) {
    page->name = "ATOMS OS Login Screen";
    page->nav_next_id = ROOK_PAGE_DESKTOP;
    return 0;
}

static int login_on_enter(rook_page_t* page) {
    (void)page;
    g_login_ticks = 0;
    s_password_buf[0] = '\0';
    s_password_len = 0;
    s_error_state = false;
    s_show_password = false;
    s_caret_timer = 0;
    s_caret_visible = true;
    s_input_focused = true;
    s_btn_hovered = false;
    s_btn_pressed = false;
    s_auth_success_loading = false;
    s_loading_timer = 0;
    s_error_timer = 0;
    s_error_opacity = 0;
    s_cache_valid = false;
    
    if (s_shake_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_shake_handle);
        s_shake_handle = AME_INVALID_HANDLE;
    }
    if (s_btn_hover_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_btn_hover_handle);
        s_btn_hover_handle = AME_INVALID_HANDLE;
    }
    if (s_btn_press_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_btn_press_handle);
        s_btn_press_handle = AME_INVALID_HANDLE;
    }
    if (s_error_fade_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_error_fade_handle);
        s_error_fade_handle = AME_INVALID_HANDLE;
    }

    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0) width = 1024;
    if (height == 0) height = 768;
    
    if (!s_login_cache_buffer) {
        s_login_cache_buffer = (uint32_t*)kmalloc(width * height * sizeof(uint32_t));
    }

    BWE_Rect full_screen = { 0, 0, (int32_t)width, (int32_t)height };
    BWE_AddCompositorDirtyRect(&full_screen);
    return 0;
}

static int login_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    g_login_ticks += (uint32_t)delta_ms;
    
    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0) width = 1024;
    if (height == 0) height = 768;
    int center_x = width / 2;
    int card_y = 205;
    BWE_Rect input_box_rect = { center_x - 155, card_y + 165, 310, 52 };
    BWE_Rect btn_rect = { center_x - 85, card_y + 245, 170, 46 };
    BWE_Rect card_rect = { center_x - 235, card_y - 5, 470, 330 };

    if (s_input_focused && !s_auth_success_loading) {
        s_caret_timer += (uint32_t)delta_ms;
        if (s_caret_timer >= 500) {
            s_caret_timer = 0;
            s_caret_visible = !s_caret_visible;
            BWE_AddCompositorDirtyRect(&input_box_rect);
        }
    }

    if (s_auth_success_loading) {
        if (s_loading_timer > delta_ms) {
            s_loading_timer -= (uint32_t)delta_ms;
            BWE_AddCompositorDirtyRect(&btn_rect);
        } else {
            s_auth_success_loading = false;
            s_loading_timer = 0;
            extern void Desktop_Shell_StartBootExperience(void);
            Desktop_Shell_StartBootExperience();
            return 0;
        }
    }

    if (s_error_state && s_error_timer > 0) {
        if (s_error_timer > delta_ms) {
            s_error_timer -= (uint32_t)delta_ms;
            if (s_error_fade_handle != AME_INVALID_HANDLE && AME_GetState(s_error_fade_handle) == AME_STATE_RUNNING) {
                s_error_opacity = AME_GetCurrentValue(s_error_fade_handle);
                BWE_AddCompositorDirtyRect(&card_rect);
            }
        } else {
            s_error_timer = 0;
            if (s_error_fade_handle != AME_INVALID_HANDLE) {
                AME_DestroyAnimation(s_error_fade_handle);
            }
            s_error_fade_handle = AME_CreateAnimation(NULL, PROP_OPACITY, s_error_opacity, 0, 300, EASE_OUT_CUBIC);
            AME_Play(s_error_fade_handle);
            BWE_AddCompositorDirtyRect(&card_rect);
        }
    } else if (s_error_state && s_error_timer == 0) {
        if (s_error_fade_handle != AME_INVALID_HANDLE) {
            if (AME_GetState(s_error_fade_handle) == AME_STATE_RUNNING) {
                s_error_opacity = AME_GetCurrentValue(s_error_fade_handle);
                BWE_AddCompositorDirtyRect(&card_rect);
            } else {
                s_error_state = false;
                s_error_opacity = 0;
                AME_DestroyAnimation(s_error_fade_handle);
                s_error_fade_handle = AME_INVALID_HANDLE;
                BWE_AddCompositorDirtyRect(&card_rect);
            }
        } else {
            s_error_state = false;
            BWE_AddCompositorDirtyRect(&card_rect);
        }
    }

    if (s_btn_hover_handle != AME_INVALID_HANDLE && AME_GetState(s_btn_hover_handle) == AME_STATE_RUNNING) {
        BWE_AddCompositorDirtyRect(&btn_rect);
    }
    if (s_btn_press_handle != AME_INVALID_HANDLE && AME_GetState(s_btn_press_handle) == AME_STATE_RUNNING) {
        BWE_AddCompositorDirtyRect(&btn_rect);
    }
    if (s_shake_handle != AME_INVALID_HANDLE && AME_GetState(s_shake_handle) == AME_STATE_RUNNING) {
        BWE_AddCompositorDirtyRect(&card_rect);
    }

    return 0;
}

static int login_on_render(rook_page_t* page, uint32_t* fb, uint32_t stride) {
    (void)page;
    (void)stride;
    /* STEP 14 TEMPORARY INSTRUMENTATION */
    uint64_t t_total_start = step14_rdtsc();
    uint64_t t_bg_start = 0, t_bg_end = 0;
    uint64_t t_pass_start = 0, t_pass_end = 0;
    uint64_t t_eye_start = 0, t_eye_end = 0;
    uint64_t t_btn_start = 0, t_btn_end = 0;
    /* END STEP 14 */
    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0) width = 1024;
    if (height == 0) height = 768;
    int center_x = width / 2;
    int card_y = 205;

    int shake_offset = 0;
    if (s_shake_handle != AME_INVALID_HANDLE) {
        if (AME_GetState(s_shake_handle) == AME_STATE_RUNNING) {
            shake_offset = AME_GetCurrentValue(s_shake_handle);
        } else if (AME_GetState(s_shake_handle) == AME_STATE_IDLE || AME_GetState(s_shake_handle) == AME_STATE_COMPLETED || AME_GetState(s_shake_handle) == AME_STATE_CANCELLED) {
            AME_DestroyAnimation(s_shake_handle);
            s_shake_handle = AME_INVALID_HANDLE;
        }
    }

    /* 1. DWM Cache Initialization / Static Pass */
    if (!s_cache_valid && s_login_cache_buffer) {
        if (g_wallpaper_loaded && g_wallpaper_buffer != NULL) {
            uint32_t total_pixels = width * height;
            for (uint32_t i = 0; i < total_pixels; i++) {
                s_login_cache_buffer[i] = g_wallpaper_buffer[i];
            }
        } else {
            draw_visible_spiral_galaxy(s_login_cache_buffer, width, height);
        }
        /* [OPTION B] Removed decorative header elements to restore clean Login UI appearance:
        login_draw_atom_logo(s_login_cache_buffer, width, height, center_x, 80, 56, 18, 10, 50);
        draw_centered_str(s_login_cache_buffer, width, height, "ATOMS OS", 130, 0xFFFFFFFF, 3, 16, 0);
        draw_centered_str(s_login_cache_buffer, width, height, "ENGINEERED FOR THE FUTURE", 165, 0xFF888888, 1, 6, 0);
        */

        int card_w = 460;
        int card_h = 320;
        int card_x = center_x - card_w / 2;
        
        draw_box(s_login_cache_buffer, width, height, card_x - 1, card_y - 1, card_w + 2, card_h + 2, 0xFF222222, 0);
        draw_box(s_login_cache_buffer, width, height, card_x, card_y, card_w, card_h, 0xFF1A1A1A, 0xFF060606);

        login_fill_circle(s_login_cache_buffer, width, height, center_x, card_y + 45, 28, 0xFF0A0A0A);
        login_fill_circle(s_login_cache_buffer, width, height, center_x, card_y + 45, 27, 0xFF282828);
        login_draw_atom_logo(s_login_cache_buffer, width, height, center_x, card_y + 45, 24, 8, 4, 20);

        draw_centered_str(s_login_cache_buffer, width, height, Identity_GetDefaultUsername(), card_y + 95, 0xFFFFFFFF, 2, 6, 0);
        draw_centered_str(s_login_cache_buffer, width, height, "ATOMS OS SYSTEM ACCOUNT", card_y + 130, 0xFF888888, 1, 3, 0);
        draw_centered_str(s_login_cache_buffer, width, height, "SHUTDOWN     RESTART     OPTIONS", height - 40, 0xFF555555, 1, 4, 0);

        s_cache_valid = true;
    }

    /* 2. Dirty Rectangle Background Restore */
    if (g_dirty_rect_count == 0 && shake_offset == 0) {
        return 0;
    }
    /* STEP 14 */ t_bg_start = step14_rdtsc(); /* END STEP 14 */

    if (shake_offset == 0 && s_login_cache_buffer && s_cache_valid) {
        for (uint32_t d = 0; d < g_dirty_rect_count; d++) {
            BWE_Rect r = g_dirty_rects[d];
            int32_t x1 = r.x;
            int32_t y1 = r.y;
            int32_t x2 = r.x + r.width;
            int32_t y2 = r.y + r.height;

            if (x1 < 0) x1 = 0;
            if (y1 < 0) y1 = 0;
            if (x2 > (int32_t)width) x2 = (int32_t)width;
            if (y2 > (int32_t)height) y2 = (int32_t)height;

            if (x1 >= x2 || y1 >= y2) continue;

            for (int32_t py = y1; py < y2; py++) {
                uint32_t* dst_row = fb + (py * width);
                uint32_t* src_row = s_login_cache_buffer + (py * width);
                for (int32_t px = x1; px < x2; px++) {
                    dst_row[px] = src_row[px];
                }
            }
        }
    } else {
        /* When shaking or cache uninitialized, draw full background and card at offset */
        if (g_wallpaper_loaded && g_wallpaper_buffer != NULL) {
            uint32_t total_pixels = width * height;
            for (uint32_t i = 0; i < total_pixels; i++) {
                fb[i] = g_wallpaper_buffer[i];
            }
        } else {
            draw_visible_spiral_galaxy(fb, width, height);
        }
        /* [OPTION B] Removed decorative header elements to restore clean Login UI appearance:
        login_draw_atom_logo(fb, width, height, center_x, 80, 56, 18, 10, 50);
        draw_centered_str(fb, width, height, "ATOMS OS", 130, 0xFFFFFFFF, 3, 16, 0);
        draw_centered_str(fb, width, height, "ENGINEERED FOR THE FUTURE", 165, 0xFF888888, 1, 6, 0);
        */

        int card_w = 460;
        int card_h = 320;
        int card_x = center_x - card_w / 2 + shake_offset;
        
        draw_box(fb, width, height, card_x - 1, card_y - 1, card_w + 2, card_h + 2, 0xFF222222, 0);
        draw_box(fb, width, height, card_x, card_y, card_w, card_h, 0xFF1A1A1A, 0xFF060606);

        login_fill_circle(fb, width, height, center_x + shake_offset, card_y + 45, 28, 0xFF0A0A0A);
        login_fill_circle(fb, width, height, center_x + shake_offset, card_y + 45, 27, 0xFF282828);
        login_draw_atom_logo(fb, width, height, center_x + shake_offset, card_y + 45, 24, 8, 4, 20);

        draw_centered_str(fb, width, height, Identity_GetDefaultUsername(), card_y + 95, 0xFFFFFFFF, 2, 6, shake_offset);
        draw_centered_str(fb, width, height, "ATOMS OS SYSTEM ACCOUNT", card_y + 130, 0xFF888888, 1, 3, shake_offset);
        draw_centered_str(fb, width, height, "SHUTDOWN     RESTART     OPTIONS", height - 40, 0xFF555555, 1, 4, 0);
    }
    /* STEP 14 */ t_bg_end = step14_rdtsc(); t_pass_start = step14_rdtsc(); /* END STEP 14 */

    /* 3. Dynamic Controls Pass */
    int input_w = 300;
    int input_h = 42;
    int input_x = center_x - input_w / 2 + shake_offset;
    int input_y = card_y + 170;
    
    uint32_t border_col = s_error_state ? 0xFFFF4444 : (s_input_focused ? 0xFF00AACC : 0xFF333333);
    draw_box(fb, width, height, input_x - 1, input_y - 1, input_w + 2, input_h + 2, border_col, 0);
    draw_box(fb, width, height, input_x, input_y, input_w, input_h, border_col, 0xFF040404);

    int text_len = 0;
    while (s_password_buf[text_len]) text_len++;

    if (text_len == 0) {
        draw_centered_str(fb, width, height, "ENTER PASSWORD...", input_y + 14, 0xFF666666, 1, 4, shake_offset);
        if (s_input_focused && s_caret_visible && !s_auth_success_loading) {
            int caret_x = center_x - 76 + shake_offset;
            for (int cy = input_y + 11; cy <= input_y + 31; cy++) {
                login_putpixel(fb, width, height, caret_x, cy, 0xFFFFFFFF);
                login_putpixel(fb, width, height, caret_x + 1, cy, 0xFFFFFFFF);
            }
        }
    } else {
        char display_str[128] = "";
        int m_idx = 0;
        if (s_show_password) {
            for (int i = 0; i < text_len && i < 30; i++) {
                display_str[m_idx++] = s_password_buf[i];
            }
        } else {
            for (int i = 0; i < text_len && i < 30; i++) {
                display_str[m_idx++] = '*';
                display_str[m_idx++] = ' ';
            }
            if (m_idx > 0) m_idx--;
        }
        display_str[m_idx] = '\0';
        
        draw_centered_str(fb, width, height, display_str, input_y + 14, 0xFFFFFFFF, 1, 4, shake_offset);
        
        if (s_input_focused && s_caret_visible && !s_auth_success_loading) {
            int str_chars = 0;
            while (display_str[str_chars]) str_chars++;
            int total_w = str_chars * 8 + (str_chars - 1) * 4;
            int start_x = (width - total_w) / 2 + shake_offset;
            int caret_x = start_x + total_w + 6;
            for (int cy = input_y + 11; cy <= input_y + 31; cy++) {
                login_putpixel(fb, width, height, caret_x, cy, 0xFFFFFFFF);
                login_putpixel(fb, width, height, caret_x + 1, cy, 0xFFFFFFFF);
            }
        }
    }
    /* STEP 14 */ t_pass_end = step14_rdtsc(); t_eye_start = step14_rdtsc(); /* END STEP 14 */

    /* Show Password Eye Icon */
    int eye_x = input_x + input_w - 24;
    int eye_y = input_y + 21;
    uint32_t eye_col = s_show_password ? 0xFF00DDFF : 0xFF777777;
    login_fill_circle(fb, width, height, eye_x, eye_y, 8, 0xFF222222);
    login_fill_circle(fb, width, height, eye_x, eye_y, 7, eye_col);
    login_fill_circle(fb, width, height, eye_x, eye_y, 4, 0xFF040404);
    login_fill_circle(fb, width, height, eye_x, eye_y, 2, s_show_password ? 0xFFFFFFFF : 0xFF888888);
    if (!s_show_password) {
        for (int i = -7; i <= 7; i++) {
            login_putpixel(fb, width, height, eye_x + i, eye_y - i, 0xFFCCCCCC);
            login_putpixel(fb, width, height, eye_x + i + 1, eye_y - i, 0xFFCCCCCC);
        }
    }
    /* STEP 14 */ t_eye_end = step14_rdtsc(); t_btn_start = step14_rdtsc(); /* END STEP 14 */

    /* Error Message */
    if (s_error_state && s_error_opacity > 0) {
        uint32_t err_col = 0xFF000000 | (s_error_opacity << 16) | ((0x44 * s_error_opacity / 255) << 8) | (0x44 * s_error_opacity / 255);
        draw_centered_str(fb, width, height, "INVALID USERNAME OR PASSWORD.", input_y + 52, err_col, 1, 2, shake_offset);
    }

    /* LOGIN Button */
    int btn_x = center_x - 80 + shake_offset;
    int btn_y = card_y + 250;
    int btn_w = 160;
    int btn_h = 36;
    
    if (s_btn_press_handle != AME_INVALID_HANDLE && AME_GetState(s_btn_press_handle) == AME_STATE_RUNNING) {
        btn_y += AME_GetCurrentValue(s_btn_press_handle);
    }
    
    int btn_opacity = 128;
    if (s_btn_hover_handle != AME_INVALID_HANDLE && AME_GetState(s_btn_hover_handle) == AME_STATE_RUNNING) {
        btn_opacity = AME_GetCurrentValue(s_btn_hover_handle);
    } else if (s_btn_hovered) {
        btn_opacity = 255;
    }
    
    uint32_t btn_border = 0xFF555555;
    uint32_t btn_fill = 0xFF181818;
    const char* btn_text = "LOGIN ->";
    uint32_t text_col = 0xFFFFFFFF;
    
    if (s_auth_success_loading) {
        btn_border = 0xFF00AACC;
        btn_fill = 0xFF002233;
        btn_text = "AUTHENTICATING...";
        text_col = 0xFF00DDFF;
    } else if (btn_opacity > 128) {
        uint32_t hl = (uint32_t)(0x55 + (0xAA * (btn_opacity - 128) / 127));
        btn_border = 0xFF000000 | (hl << 16) | (hl << 8) | hl;
        btn_fill = 0xFF222222;
    }
    
    draw_box(fb, width, height, btn_x, btn_y, btn_w, btn_h, btn_border, btn_fill);
    draw_centered_str(fb, width, height, btn_text, btn_y + 11, text_col, 1, 4, shake_offset);
    /* STEP 14 TEMPORARY INSTRUMENTATION */
    t_btn_end = step14_rdtsc();
    uint64_t t_total_end = step14_rdtsc();
    step14_log_login_render(
        step14_cycles_to_us(t_total_end - t_total_start),
        step14_cycles_to_us(t_bg_end - t_bg_start),
        step14_cycles_to_us(t_total_end - t_pass_start),
        step14_cycles_to_us(t_eye_end - t_eye_start),
        step14_cycles_to_us(t_pass_end - t_pass_start),
        step14_cycles_to_us(t_btn_end - t_btn_start)
    );
    /* END STEP 14 */

    return 0;
}

static rook_page_t g_page_login = {
    .id = ROOK_PAGE_LOGIN,
    .state = ROOK_STATE_UNALLOCATED,
    .ops = {
        .on_create = login_on_create,
        .on_enter  = login_on_enter,
        .on_update = login_on_update,
        .on_render = login_on_render
    }
};

rook_page_t* rook_page_login_get(void) {
    return &g_page_login;
}
