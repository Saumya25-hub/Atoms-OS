#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "../../libbos_gui/include/syscalls_gui.h"

static void bos_print_dec(uint64_t num) {
    if (num == 0) {
        bos_print("0");
        return;
    }
    char buf[32];
    int i = 0;
    while (num > 0) {
        buf[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = tmp;
    }
    buf[i] = '\0';
    bos_print(buf);
}

// 8x8 Simple Font Bitmaps for Clean Userspace Rendering
static const uint8_t g_font8x8_basic[128][8] = {
    [' '] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!'] = {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00},
    ['A'] = {0x7C,0x12,0x11,0x12,0x7C,0x00,0x00,0x00},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00},
    ['D'] = {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00},
    ['H'] = {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00},
    ['R'] = {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00},
    ['S'] = {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00},
    ['T'] = {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00},
    ['V'] = {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00},
    ['W'] = {0x7F,0x20,0x18,0x20,0x7F,0x00,0x00,0x00},
    ['X'] = {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07,0x00,0x00,0x00},
    ['Z'] = {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00},
    ['a'] = {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00},
    ['b'] = {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00},
    ['c'] = {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00},
    ['d'] = {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00},
    ['e'] = {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00},
    ['f'] = {0x08,0x7E,0x09,0x01,0x02,0x00,0x00,0x00},
    ['g'] = {0x08,0x14,0x54,0x54,0x3C,0x00,0x00,0x00},
    ['h'] = {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['i'] = {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00},
    ['j'] = {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00},
    ['k'] = {0x7F,0x10,0x28,0x44,0x00,0x00,0x00,0x00},
    ['l'] = {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['m'] = {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00},
    ['n'] = {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00},
    ['o'] = {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00},
    ['p'] = {0x7C,0x14,0x14,0x14,0x08,0x00,0x00,0x00},
    ['q'] = {0x08,0x14,0x14,0x18,0x7C,0x00,0x00,0x00},
    ['r'] = {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00},
    ['s'] = {0x48,0x54,0x54,0x54,0x20,0x00,0x00,0x00},
    ['t'] = {0x04,0x3F,0x44,0x40,0x20,0x00,0x00,0x00},
    ['u'] = {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00},
    ['v'] = {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00},
    ['w'] = {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00},
    ['x'] = {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00},
    ['y'] = {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00},
    ['z'] = {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00},
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00},
    ['1'] = {0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00},
    ['3'] = {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00},
    ['5'] = {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00},
    ['7'] = {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00},
    ['8'] = {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00},
    ['9'] = {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00},
    [':'] = {0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00},
    ['-'] = {0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00},
    ['.'] = {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00},
    ['['] = {0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00},
    [']'] = {0x00,0x41,0x41,0x7F,0x00,0x00,0x00,0x00}
};

static void draw_char(uint32_t* fb, uint32_t stride, char c, int x, int y, uint32_t color) {
    if ((uint8_t)c > 127) return;
    for (int col = 0; col < 6; col++) {
        uint8_t line = g_font8x8_basic[(uint8_t)c][col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                fb[(y + row) * stride + (x + col)] = color;
            }
        }
    }
}

static void draw_string(uint32_t* fb, uint32_t stride, const char* str, int x, int y, uint32_t color) {
    int cur_x = x;
    while (*str) {
        draw_char(fb, stride, *str, cur_x, y, color);
        cur_x += 7;
        str++;
    }
}

static void fill_rect(uint32_t* fb, uint32_t stride, int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h; py++) {
        uint32_t row = py * stride;
        for (int px = x; px < x + w; px++) {
            fb[row + px] = color;
        }
    }
}

#include "desktop_icon_data.h"

static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static void draw_rounded_glass_rect(uint32_t* fb, uint32_t stride, int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color) {
    uint32_t bg_a = (bg_color >> 24) & 0xFF;
    uint32_t bg_r = (bg_color >> 16) & 0xFF;
    uint32_t bg_g = (bg_color >> 8) & 0xFF;
    uint32_t bg_b = bg_color & 0xFF;

    for (int py = y; py < y + h; py++) {
        uint32_t row = py * stride;
        for (int px = x; px < x + w; px++) {
            bool is_border = (px == x || px == x + w - 1 || py == y || py == y + h - 1);
            if (is_border) {
                fb[row + px] = border_color;
            } else if (bg_a > 0) {
                uint32_t dst = fb[row + px];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t out_r = (bg_r * bg_a + dr * (255 - bg_a)) / 255;
                uint32_t out_g = (bg_g * bg_a + dg * (255 - bg_a)) / 255;
                uint32_t out_b = (bg_b * bg_a + db * (255 - bg_a)) / 255;
                fb[row + px] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
            }
        }
    }
}

static void draw_desktop_icon(uint32_t* surface, uint32_t stride, uint32_t screen_w, uint32_t screen_h,
                              int32_t dst_x, int32_t dst_y, int32_t dst_w, int32_t dst_h,
                              const uint32_t* src_bitmap, bool is_hovered, bool is_selected) {
    if (!surface || !src_bitmap || dst_w <= 0 || dst_h <= 0) return;

    const int32_t src_w = DESKTOP_ICON_MASTER_SIZE;
    const int32_t src_h = DESKTOP_ICON_MASTER_SIZE;

    uint32_t step_x = ((src_w - 1) << 16) / dst_w;
    uint32_t step_y = ((src_h - 1) << 16) / dst_h;

    for (int32_t dy = 0; dy < dst_h; dy++) {
        int32_t py = dst_y + dy;
        if (py < 0 || py >= (int32_t)screen_h) continue;

        uint32_t src_fix_y = dy * step_y;
        int32_t sy0 = (int32_t)(src_fix_y >> 16);
        int32_t sy1 = (sy0 + 1 < src_h) ? sy0 + 1 : sy0;
        uint32_t frac_y = (src_fix_y & 0xFFFF) >> 8;
        uint32_t inv_frac_y = 256 - frac_y;

        uint32_t row_offset = py * stride;

        for (int32_t dx = 0; dx < dst_w; dx++) {
            int32_t px = dst_x + dx;
            if (px < 0 || px >= (int32_t)screen_w) continue;

            uint32_t src_fix_x = dx * step_x;
            int32_t sx0 = (int32_t)(src_fix_x >> 16);
            int32_t sx1 = (sx0 + 1 < src_w) ? sx0 + 1 : sx0;
            uint32_t frac_x = (src_fix_x & 0xFFFF) >> 8;
            uint32_t inv_frac_x = 256 - frac_x;

            uint32_t p00 = src_bitmap[sy0 * src_w + sx0];
            uint32_t p10 = src_bitmap[sy0 * src_w + sx1];
            uint32_t p01 = src_bitmap[sy1 * src_w + sx0];
            uint32_t p11 = src_bitmap[sy1 * src_w + sx1];

            uint32_t a00 = (p00 >> 24) & 0xFF, r00 = (p00 >> 16) & 0xFF, g00 = (p00 >> 8) & 0xFF, b00 = p00 & 0xFF;
            uint32_t a10 = (p10 >> 24) & 0xFF, r10 = (p10 >> 16) & 0xFF, g10 = (p10 >> 8) & 0xFF, b10 = p10 & 0xFF;
            uint32_t a01 = (p01 >> 24) & 0xFF, r01 = (p01 >> 16) & 0xFF, g01 = (p01 >> 8) & 0xFF, b01 = p01 & 0xFF;
            uint32_t a11 = (p11 >> 24) & 0xFF, r11 = (p11 >> 16) & 0xFF, g11 = (p11 >> 8) & 0xFF, b11 = p11 & 0xFF;

            uint32_t top_a = (a00 * inv_frac_x + a10 * frac_x) >> 8;
            uint32_t top_r = (r00 * inv_frac_x + r10 * frac_x) >> 8;
            uint32_t top_g = (g00 * inv_frac_x + g10 * frac_x) >> 8;
            uint32_t top_b = (b00 * inv_frac_x + b10 * frac_x) >> 8;

            uint32_t bot_a = (a01 * inv_frac_x + a11 * frac_x) >> 8;
            uint32_t bot_r = (r01 * inv_frac_x + r11 * frac_x) >> 8;
            uint32_t bot_g = (g01 * inv_frac_x + g11 * frac_x) >> 8;
            uint32_t bot_b = (b01 * inv_frac_x + b11 * frac_x) >> 8;

            uint32_t a = (top_a * inv_frac_y + bot_a * frac_y) >> 8;
            if (a == 0) continue;

            uint32_t r = (top_r * inv_frac_y + bot_r * frac_y) >> 8;
            uint32_t g = (top_g * inv_frac_y + bot_g * frac_y) >> 8;
            uint32_t b = (top_b * inv_frac_y + bot_b * frac_y) >> 8;

            if (is_selected) {
                // Focus cyan tint
                r = clamp_u8((r * 220 + 56 * 35) / 255);
                g = clamp_u8((g * 220 + 189 * 35) / 255);
                b = clamp_u8((b * 220 + 248 * 35) / 255);
            } else if (is_hovered) {
                r = clamp_u8((int)r + 28);
                g = clamp_u8((int)g + 28);
                b = clamp_u8((int)b + 28);
            }

            uint32_t buf_idx = row_offset + px;
            if (a >= 250) {
                surface[buf_idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else {
                uint32_t dst = surface[buf_idx];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t out_r = (r * a + dr * (255 - a)) / 255;
                uint32_t out_g = (g * a + dg * (255 - a)) / 255;
                uint32_t out_b = (b * a + db * (255 - a)) / 255;
                surface[buf_idx] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
            }
        }
    }
}

// Desktop State
typedef struct {
    const char* name;
    int x, y, w, h;
    const uint32_t* bitmap;
    bool selected;
} DesktopIcon;

static DesktopIcon g_icons[] = {
    { "Computer", 24, 24,  76, 76, g_desktop_ico_computer_48, false },
    { "Files",    24, 114, 76, 76, g_desktop_ico_files_48,    false },
    { "Terminal", 24, 204, 76, 76, g_desktop_ico_terminal_48, false },
    { "Settings", 24, 294, 76, 76, g_desktop_ico_settings_48, false }
};

static void draw_string_with_shadow(uint32_t* fb, uint32_t stride, const char* str, int x, int y, uint32_t color) {
    draw_string(fb, stride, str, x + 1, y + 1, 0xCC000000); // Soft drop shadow
    draw_string(fb, stride, str, x, y, color);               // Crisp text
}

static void render_desktop(uint32_t win_id, uint32_t* surface, uint32_t width, uint32_t height) {
    // 1. Wallpaper Background (Rendered via Syscall 24 across full display)
    if (sys_gui_draw_wallpaper(win_id, 0, 0, (int32_t)width, (int32_t)height) != 0) {
        // Fallback to Deep Midnight Dark Slate if Syscall fails
        for (uint32_t y = 0; y < height; y++) {
            uint32_t row = y * width;
            uint32_t bg = (y < height / 2) ? 0xFF0B1120 : 0xFF0F172A;
            for (uint32_t x = 0; x < width; x++) {
                surface[row + x] = bg;
            }
        }
    }

    // 2. Render Official PNG Desktop Icons
    for (int i = 0; i < 4; i++) {
        int ix = g_icons[i].x;
        int iy = g_icons[i].y;
        int iw = g_icons[i].w;
        int ih = g_icons[i].h;
        
        // Translucent Glass Selection & Hover Tile
        if (g_icons[i].selected) {
            draw_rounded_glass_rect(surface, width, ix, iy, iw, ih, 0x3338BDF8, 0x9938BDF8);
        }

        // Render Official PNG Icon (40x40 Centered, Bilinear Scaled with Sub-pixel Alpha)
        int icon_size = 40;
        int icon_x = ix + (iw - icon_size) / 2;
        int icon_y = iy + 4;
        draw_desktop_icon(surface, width, width, height, icon_x, icon_y, icon_size, icon_size, g_icons[i].bitmap, false, g_icons[i].selected);
        
        // Render Centered Typography Label with Drop Shadow
        int name_len = 0;
        while (g_icons[i].name[name_len] != '\0') name_len++;
        int text_x = ix + (iw - name_len * 8) / 2;
        int text_y = iy + 52;
        draw_string_with_shadow(surface, width, g_icons[i].name, text_x, text_y, 0xFFF8FAFC);
    }
}

void main(void) {
    bos_print("[LOGIN_FLOW] DESKTOP_FIRST_RUN PID=200\r\n");
    bos_print("\r\n[DESKTOP] PROCESS ENTRY REACHED\r\n[DESKTOP] PID=200\r\n[DESKTOP] CPL=3\r\n");

    uint32_t scr_w = 1024, scr_h = 768, scr_bpp = 32;
    sys_gui_get_screen_info(&scr_w, &scr_h, &scr_bpp);
    if (scr_w == 0 || scr_h == 0) {
        scr_w = 1024;
        scr_h = 768;
    }

    bos_print("[LOGIN_FLOW] DESKTOP_INIT_BEGIN\r\n");
    BOS_GUI_Init();

    // Create fullscreen borderless Desktop Window (flags=1: BWE_WINDOW_BORDERLESS)
    uint32_t win_id = sys_gui_create_window(0, 0, scr_w, scr_h, 1, "ATOMS Desktop Shell");
    if (!win_id) {
        bos_print("[DESKTOP] WINDOW CREATE FAIL\r\n");
        bos_exit();
    }
    bos_print("[LOGIN_FLOW] DESKTOP_INIT_OK\r\n");
    bos_print("[DESKTOP] WINDOW CREATED\r\n");

    uint32_t* surface = 0;
    uint32_t stride = 0;
    bos_print("[LOGIN_FLOW] COMPOSITOR_REGISTER_BEGIN\r\n");
    if (sys_gui_map_surface(win_id, &surface, &stride) == 0 && surface) {
        bos_print("[LOGIN_FLOW] COMPOSITOR_REGISTER_OK\r\n");
        bos_print("[DESKTOP] SURFACE MAPPED\r\n");

        render_desktop(win_id, surface, scr_w, scr_h);
        bos_print("[DESKTOP] DESKTOP RENDERED\r\n");

        bos_print("[LOGIN_FLOW] DESKTOP_PRESENT_BEGIN\r\n");
        sys_gui_invalidate(win_id, 0, 0, scr_w, scr_h);
        bos_print("[LOGIN_FLOW] DESKTOP_PRESENT_OK\r\n");
        bos_print("[DESKTOP] INVALIDATE PASS\r\n");
    } else {
        bos_print("[DESKTOP] SURFACE MAP FAIL\r\n");
    }

    sys_gui_show_window(win_id, true);
    bos_print("[DESKTOP] SHOW_WINDOW PASS\r\n");

    static BOS_GUIEvent event;
    while (1) {
        if (sys_gui_poll_event(win_id, &event)) {
            static int s_prev_hovered_icon = -1;
            int dirty_x = 0, dirty_y = 0, dirty_w = 0, dirty_h = 0;
            bool need_redraw = false;

            if (event.type == BOS_GUI_EVENT_MOUSE_MOVE) {
                int mx = event.mouse_x;
                int my = event.mouse_y;
                for (int i = 0; i < 4; i++) {
                    bool hover = (mx >= g_icons[i].x && mx < g_icons[i].x + g_icons[i].w &&
                                  my >= g_icons[i].y && my < g_icons[i].y + g_icons[i].h);
                    if (g_icons[i].selected != hover) {
                        g_icons[i].selected = hover;
                        need_redraw = true;
                        if (s_prev_hovered_icon >= 0 && s_prev_hovered_icon < 4 && s_prev_hovered_icon != i) {
                            sys_gui_invalidate(win_id, g_icons[s_prev_hovered_icon].x, g_icons[s_prev_hovered_icon].y, g_icons[s_prev_hovered_icon].w, g_icons[s_prev_hovered_icon].h);
                        }
                        sys_gui_invalidate(win_id, g_icons[i].x, g_icons[i].y, g_icons[i].w, g_icons[i].h);
                        s_prev_hovered_icon = hover ? i : -1;
                    }
                }
            } else if (event.type == BOS_GUI_EVENT_MOUSE_DOWN) {
                bos_print("[DESKTOP] MOUSE CLICK RECEIVED\r\n");
                int mx = event.mouse_x;
                int my = event.mouse_y;

                // Check desktop icons
                for (int i = 0; i < 4; i++) {
                    if (mx >= g_icons[i].x && mx < g_icons[i].x + g_icons[i].w &&
                        my >= g_icons[i].y && my < g_icons[i].y + g_icons[i].h) {
                        g_icons[i].selected = true;
                        need_redraw = true;
                        dirty_x = g_icons[i].x; dirty_y = g_icons[i].y; dirty_w = g_icons[i].w; dirty_h = g_icons[i].h;
                        bos_print("[DESKTOP] ICON CLICKED: ");
                        bos_print(g_icons[i].name);
                        bos_print("\r\n");
                    }
                }
            } else if (event.type == BOS_GUI_EVENT_KEY_DOWN) {
                bos_print("[DESKTOP] KEY EVENT RECEIVED\r\n");
            }

            if (need_redraw && surface) {
                render_desktop(win_id, surface, scr_w, scr_h);
                if (dirty_w > 0 && dirty_h > 0) {
                    sys_gui_invalidate(win_id, dirty_x, dirty_y, dirty_w, dirty_h);
                } else {
                    sys_gui_invalidate(win_id, 0, 0, scr_w, scr_h);
                }
            }
        } else {
            bos_yield();
        }
    }

    bos_exit();
}

void _start(void) {
    main();
    bos_exit();
}
