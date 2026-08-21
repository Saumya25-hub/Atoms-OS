#include "../../libbos/include/bos.h"
#include "../../libbos_gui/include/syscalls_gui.h"
#include "../../../bovisual/Text/font8x16.h"

/*
 * =====================================================================
 * ATOMS OS — NEW RING 3 DESKTOP APPLICATION (PHASE 1)
 * =====================================================================
 * Single Fullscreen Window + Single Shared Surface + Static Taskbar
 * CPL=3 Userspace Authority
 * =====================================================================
 */

#define COLOR_DESKTOP_BG    0xFF0B1120 /* Deep Midnight Slate */
#define COLOR_TASKBAR_BG    0xFF0F172A /* Dark Slate Taskbar */
#define COLOR_TASKBAR_LINE  0xFF334155 /* Top Border Line */
#define COLOR_START_BTN     0xFF2563EB /* Royal Blue Start Button */
#define COLOR_START_TEXT    0xFFFFFFFF /* Crisp White Text */
#define COLOR_CLOCK_BG      0xFF1E293B /* Dark Glass Clock Widget */
#define COLOR_CLOCK_TEXT    0xFF94A3B8 /* Soft Muted Slate Text */

#define TASKBAR_HEIGHT      44

static void draw_rect(uint32_t *fb, uint32_t scr_w, uint32_t scr_h,
                      int x, int y, int w, int h, uint32_t color) {
    if (!fb || x >= (int)scr_w || y >= (int)scr_h) return;
    int x2 = x + w;
    int y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > (int)scr_w) x2 = (int)scr_w;
    if (y2 > (int)scr_h) y2 = (int)scr_h;

    for (int cy = y; cy < y2; cy++) {
        uint32_t *row = &fb[cy * scr_w];
        for (int cx = x; cx < x2; cx++) {
            row[cx] = color;
        }
    }
}

static void draw_char(uint32_t *fb, uint32_t scr_w, uint32_t scr_h,
                      int x, int y, char c, uint32_t color) {
    if (!fb || x < 0 || y < 0 || (x + 8) > (int)scr_w || (y + 16) > (int)scr_h) return;
    const uint8_t *glyph = g_font8x16_stub[(uint8_t)c];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        uint32_t *dst = &fb[(y + row) * scr_w + x];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                dst[col] = color;
            }
        }
    }
}

static void draw_string(uint32_t *fb, uint32_t scr_w, uint32_t scr_h,
                        int x, int y, const char *str, uint32_t color) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        draw_char(fb, scr_w, scr_h, cur_x, y, *str, color);
        cur_x += 8;
        str++;
    }
}

static void render_desktop(uint32_t *surface, uint32_t w, uint32_t h) {
    if (!surface || w == 0 || h == 0) return;

    /* 1. Background Fill */
    uint32_t desktop_area_h = (h > TASKBAR_HEIGHT) ? (h - TASKBAR_HEIGHT) : h;
    draw_rect(surface, w, h, 0, 0, w, desktop_area_h, COLOR_DESKTOP_BG);

    /* 2. Taskbar Background Fill */
    int taskbar_y = (int)h - TASKBAR_HEIGHT;
    draw_rect(surface, w, h, 0, taskbar_y, w, TASKBAR_HEIGHT, COLOR_TASKBAR_BG);

    /* 3. Taskbar 1px Top Divider */
    draw_rect(surface, w, h, 0, taskbar_y, w, 1, COLOR_TASKBAR_LINE);

    /* 4. Start Button Visual Rectangle (x=12, y=h-38, w=84, h=32) */
    int btn_x = 12;
    int btn_y = taskbar_y + 6;
    int btn_w = 84;
    int btn_h = 32;
    draw_rect(surface, w, h, btn_x, btn_y, btn_w, btn_h, COLOR_START_BTN);
    draw_string(surface, w, h, btn_x + 22, btn_y + 8, "START", COLOR_START_TEXT);

    /* 5. Clock Visual Rectangle (x=w-114, y=h-38, w=102, h=32) */
    int clock_w = 102;
    int clock_x = (int)w - clock_w - 12;
    int clock_y = taskbar_y + 6;
    int clock_h = 32;
    draw_rect(surface, w, h, clock_x, clock_y, clock_w, clock_h, COLOR_CLOCK_BG);
    draw_string(surface, w, h, clock_x + 19, clock_y + 8, "12:00 PM", COLOR_CLOCK_TEXT);
}

void main(void) {
    bos_print("[DESKTOP] START\r\n");

    uint32_t scr_w = 1024, scr_h = 768, scr_bpp = 32;
    sys_gui_get_screen_info(&scr_w, &scr_h, &scr_bpp);
    if (scr_w == 0 || scr_h == 0) {
        scr_w = 1024;
        scr_h = 768;
    }
    bos_print("[DESKTOP] SCREEN DISCOVERED\r\n");

    /* Create single fullscreen borderless Desktop Window (flags=1: BWE_WINDOW_BORDERLESS) */
    uint32_t win_id = sys_gui_create_window(0, 0, (int32_t)scr_w, (int32_t)scr_h, 1, "ATOMS Desktop");
    if (!win_id) {
        bos_print("[DESKTOP] WINDOW CREATE FAIL\r\n");
        bos_exit();
    }
    bos_print("[DESKTOP] WINDOW CREATE PASS\r\n");

    uint32_t *surface = 0;
    uint32_t stride_bytes = 0;
    if (sys_gui_map_surface(win_id, &surface, &stride_bytes) != 0 || !surface) {
        bos_print("[DESKTOP] SURFACE MAP FAIL\r\n");
        bos_exit();
    }
    bos_print("[DESKTOP] SURFACE MAP PASS\r\n");

    /* Render Initial Simple Desktop + Taskbar */
    render_desktop(surface, scr_w, scr_h);
    bos_print("[DESKTOP] BACKGROUND DRAW PASS\r\n");
    bos_print("[DESKTOP] TASKBAR DRAW PASS\r\n");

    /* Invalidate Full Window to Trigger BWE Compositor */
    sys_gui_invalidate(win_id, 0, 0, (int32_t)scr_w, (int32_t)scr_h);
    bos_print("[DESKTOP] INVALIDATE PASS\r\n");

    /* Show Desktop Window */
    sys_gui_show_window(win_id, true);
    bos_print("[DESKTOP] EVENT LOOP ACTIVE\r\n");

    /* Continuous Interactive Event Loop */
    BOS_GUIEvent event;
    while (1) {
        if (sys_gui_poll_event(win_id, &event)) {
            /* Event received - no complex actions in Phase 1 */
        } else {
            bos_yield();
        }
    }

    bos_exit();
}

void _start(void) {
    main();
}
