#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/bos_gui.h"
#include "../../libbos_gui/include/syscalls_gui.h"

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

static void draw_border_rect(uint32_t* fb, uint32_t stride, int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color) {
    fill_rect(fb, stride, x, y, w, h, bg_color);
    for (int px = x; px < x + w; px++) {
        fb[y * stride + px] = border_color;
        fb[(y + h - 1) * stride + px] = border_color;
    }
    for (int py = y; py < y + h; py++) {
        fb[py * stride + x] = border_color;
        fb[py * stride + (x + w - 1)] = border_color;
    }
}

// Desktop State
typedef struct {
    const char* name;
    int x, y, w, h;
    uint32_t icon_color;
    bool selected;
} DesktopIcon;

static DesktopIcon g_icons[] = {
    { "Computer", 40, 40,  80, 80, 0xFF2563EB, false },
    { "Files",    40, 140, 80, 80, 0xFF10B981, false },
    { "Terminal", 40, 240, 80, 80, 0xFF38BDF8, false },
    { "Settings", 40, 340, 80, 80, 0xFFF59E0B, false }
};

static bool g_start_menu_open = false;

static void render_desktop(uint32_t* surface, uint32_t width, uint32_t height) {
    // 1. Wallpaper Background (Deep Midnight Dark Slate)
    for (uint32_t y = 0; y < height - 48; y++) {
        uint32_t row = y * width;
        uint32_t bg = (y < height / 2) ? 0xFF0B1120 : 0xFF0F172A;
        for (uint32_t x = 0; x < width; x++) {
            surface[row + x] = bg;
        }
    }

    // 2. Render Desktop Icons
    for (int i = 0; i < 4; i++) {
        int ix = g_icons[i].x;
        int iy = g_icons[i].y;
        
        // Icon card background & highlight
        uint32_t card_bg = g_icons[i].selected ? 0xFF1E293B : 0x00000000;
        uint32_t border_col = g_icons[i].selected ? 0xFF38BDF8 : 0x00000000;
        if (g_icons[i].selected) {
            draw_border_rect(surface, width, ix - 6, iy - 6, 92, 92, card_bg, border_col);
        }

        // Icon Graphic Container
        draw_border_rect(surface, width, ix + 16, iy + 6, 48, 48, g_icons[i].icon_color, 0xFFFFFFFF);
        
        // Icon Label
        draw_string(surface, width, g_icons[i].name, ix + 12, iy + 60, 0xFFF8FAFC);
    }

    // 3. Taskbar (Bottom 48px)
    int tb_y = height - 48;
    fill_rect(surface, width, 0, tb_y, width, 48, 0xFF1E293B);
    for (uint32_t x = 0; x < width; x++) surface[tb_y * width + x] = 0xFF334155; // subtle border line

    // Start Button (Blue Pill)
    uint32_t start_col = g_start_menu_open ? 0xFF1D4ED8 : 0xFF2563EB;
    draw_border_rect(surface, width, 12, tb_y + 8, 88, 32, start_col, 0xFF60A5FA);
    draw_string(surface, width, "START", 38, tb_y + 19, 0xFFFFFFFF);

    // Active Taskbar Tile
    draw_border_rect(surface, width, 110, tb_y + 8, 140, 32, 0xFF0F172A, 0xFF3B82F6);
    draw_string(surface, width, "ATOMS Desktop", 120, tb_y + 19, 0xFFE2E8F0);

    // Clock indicator on right
    draw_string(surface, width, "12:00 PM", width - 85, tb_y + 19, 0xFF94A3B8);

    // 4. Start Menu (if open)
    if (g_start_menu_open) {
        int sm_w = 260, sm_h = 320;
        int sm_x = 12, sm_y = tb_y - sm_h - 6;
        draw_border_rect(surface, width, sm_x, sm_y, sm_w, sm_h, 0xFF0F172A, 0xFF3B82F6);
        
        draw_string(surface, width, "ATOMS APPLICATIONS", sm_x + 16, sm_y + 16, 0xFF38BDF8);
        draw_string(surface, width, "-------------------", sm_x + 16, sm_y + 30, 0xFF475569);
        
        draw_string(surface, width, "1. File Explorer", sm_x + 20, sm_y + 55, 0xFFF1F5F9);
        draw_string(surface, width, "2. System Terminal", sm_x + 20, sm_y + 90, 0xFFF1F5F9);
        draw_string(surface, width, "3. System Settings", sm_x + 20, sm_y + 125, 0xFFF1F5F9);
        draw_string(surface, width, "4. Performance Hub", sm_x + 20, sm_y + 160, 0xFFF1F5F9);
        
        draw_border_rect(surface, width, sm_x + 16, sm_y + sm_h - 44, sm_w - 32, 30, 0xFFEF4444, 0xFFFCA5A5);
        draw_string(surface, width, "LOGOUT / SHUTDOWN", sm_x + 55, sm_y + sm_h - 34, 0xFFFFFFFF);
    }
}

void main(void) {
    bos_print("\r\n[DESKTOP] PROCESS ENTRY REACHED\r\n[DESKTOP] PID=200\r\n[DESKTOP] CPL=3\r\n");

    uint32_t scr_w = 1024, scr_h = 768, scr_bpp = 32;
    sys_gui_get_screen_info(&scr_w, &scr_h, &scr_bpp);
    if (scr_w == 0 || scr_h == 0) {
        scr_w = 1024;
        scr_h = 768;
    }

    BOS_GUI_Init();

    // Create fullscreen borderless Desktop Window (flags=1: BWE_WINDOW_BORDERLESS)
    uint32_t win_id = sys_gui_create_window(0, 0, scr_w, scr_h, 1, "ATOMS Desktop Shell");
    if (!win_id) {
        bos_print("[DESKTOP] WINDOW CREATE FAIL\r\n");
        bos_exit();
    }
    bos_print("[DESKTOP] WINDOW CREATED\r\n");

    uint32_t* surface = 0;
    uint32_t stride = 0;
    if (sys_gui_map_surface(win_id, &surface, &stride) == 0 && surface) {
        bos_print("[DESKTOP] SURFACE MAPPED\r\n");

        render_desktop(surface, scr_w, scr_h);
        bos_print("[DESKTOP] DESKTOP RENDERED\r\n");

        sys_gui_invalidate(win_id, 0, 0, scr_w, scr_h);
        bos_print("[DESKTOP] INVALIDATE PASS\r\n");
    } else {
        bos_print("[DESKTOP] SURFACE MAP FAIL\r\n");
    }

    sys_gui_show_window(win_id, true);
    bos_print("[DESKTOP] SHOW_WINDOW PASS\r\n");

    BOS_GUIEvent event;
    while (1) {
        if (sys_gui_poll_event(win_id, &event)) {
            bool need_redraw = false;

            if (event.type == BOS_GUI_EVENT_MOUSE_DOWN) {
                bos_print("[DESKTOP] MOUSE CLICK RECEIVED\r\n");
                int mx = event.mouse_x;
                int my = event.mouse_y;

                // Check Start Button click
                int tb_y = scr_h - 48;
                if (mx >= 12 && mx <= 100 && my >= tb_y + 8 && my <= tb_y + 40) {
                    g_start_menu_open = !g_start_menu_open;
                    need_redraw = true;
                    bos_print("[DESKTOP] START BUTTON CLICKED\r\n");
                } else {
                    // Check desktop icons
                    for (int i = 0; i < 4; i++) {
                        if (mx >= g_icons[i].x - 6 && mx <= g_icons[i].x + 86 &&
                            my >= g_icons[i].y - 6 && my <= g_icons[i].y + 86) {
                            g_icons[i].selected = true;
                            need_redraw = true;
                            bos_print("[DESKTOP] ICON CLICKED: ");
                            bos_print(g_icons[i].name);
                            bos_print("\r\n");
                        } else {
                            if (g_icons[i].selected) {
                                g_icons[i].selected = false;
                                need_redraw = true;
                            }
                        }
                    }

                    if (g_start_menu_open) {
                        g_start_menu_open = false;
                        need_redraw = true;
                    }
                }
            } else if (event.type == BOS_GUI_EVENT_KEY_DOWN) {
                bos_print("[DESKTOP] KEY EVENT RECEIVED\r\n");
            }

            if (need_redraw && surface) {
                render_desktop(surface, scr_w, scr_h);
                sys_gui_invalidate(win_id, 0, 0, scr_w, scr_h);
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
