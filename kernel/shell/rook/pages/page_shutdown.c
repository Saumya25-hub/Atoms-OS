#include "kernel/shell/rook/pages/page_shutdown.h"
#include "kernel/shell/rook/include/spinner.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/ame/include/ame.h"
#include "kernel/core/lib/include/string.h"
#include "bovisual/Include/graphics.h"
#include "bovisual/Include/text.h"
#include "kernel/ui/bofont/bofont.h"

/*
 * ♜ ATOMS OS Shutdown & Restart Experience (Page 8: ROOK_PAGE_SHUTDOWN)
 * Provides a serene, premium OS transition experience.
 * Native vector typography, centered glassmorphic status card, and fluid AME spinner.
 * Zero heap allocations, zero flicker, 100% tear-free.
 */

static rook_page_t s_shutdown_page;
static uint64_t    s_shutdown_elapsed_ms = 0;
static bool        s_shutdown_initialized = false;
static bool        s_is_restart_mode = false;

/* Offscreen static canvas cache */
static uint32_t    s_static_canvas[1920 * 1080] __attribute__((aligned(16)));
static bool        s_canvas_built = false;

void rook_page_shutdown_set_mode(bool is_restart) {
    s_is_restart_mode = is_restart;
    s_canvas_built = false; // Rebuild text for the selected mode
}

bool rook_page_shutdown_is_restart(void) {
    return s_is_restart_mode;
}

static void draw_line_thick_round(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int x0, int y0, int x1, int y1, int thickness, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int steps = (dx > dy) ? dx : dy;
    if (steps == 0) steps = 1;

    int half_t = thickness / 2;
    int r2 = half_t * half_t;

    for (int i = 0; i <= steps; i++) {
        int cx = x0 + (x1 - x0) * i / steps;
        int cy = y0 + (y1 - y0) * i / steps;

        for (int ry = -half_t; ry <= half_t; ry++) {
            int py = cy + ry;
            if (py < 0 || py >= (int)fb_h) continue;
            int ry2 = ry * ry;
            for (int rx = -half_t; rx <= half_t; rx++) {
                int px = cx + rx;
                if (px < 0 || px >= (int)fb_w) continue;
                if (rx * rx + ry2 <= r2) {
                    fb[py * stride_pixels + px] = color;
                }
            }
        }
    }
}

static inline bool is_outside_rounded_rect(int32_t x, int32_t y, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r) {
    int32_t left_cx = rx + r;
    int32_t right_cx = rx + rw - r - 1;
    int32_t top_cy = ry + r;
    int32_t bottom_cy = ry + rh - r - 1;

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

static void draw_custom_text(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int y, const char* str, uint32_t color, bool large) {
    if (!str) return;

    int len = 0;
    while (str[len]) len++;

    BVFramebuffer target_fb;
    target_fb.buffer = fb;
    target_fb.width = fb_w;
    target_fb.height = fb_h;
    target_fb.pitch = stride_pixels * 4;

    BOFontRole role = large ? BOFONT_ROLE_TITLE : BOFONT_ROLE_UI_MEDIUM;
    BOTextMetrics tm = BOFont_MeasureTextRole(role, str);
    if (tm.width > 0) {
        int text_x = cx - tm.width / 2;
        BOFont_DrawTextRoleTarget(&target_fb, role, str, text_x, y, color);
        return;
    }

    int text_x = cx - (len * 8) / 2;
    BOVISUAL_Draw_String(text_x, y, str, color, 0x00000000, true, NULL);
}

/* Pre-render static background and centered card */
static void build_static_canvas(uint32_t width, uint32_t height) {
    uint32_t total_pixels = width * height;
    if (total_pixels > (1920 * 1080)) total_pixels = 1920 * 1080;

    /* Deep Navy Blue Background */
    uint32_t bg_color = 0xFF0B1120;
    for (uint32_t i = 0; i < total_pixels; i++) {
        s_static_canvas[i] = bg_color;
    }

    int cx = (int)width / 2;
    int cy = (int)height / 2;

    /* Centered Glassmorphic Card */
    int card_w = 420;
    int card_h = 240;
    int card_x = cx - card_w / 2;
    int card_y = cy - card_h / 2;
    int card_r = 16;

    uint32_t card_bg = 0xFF0F172A;     // Slate-900
    uint32_t card_border = 0xFF334155; // Slate-700

    for (int y = card_y; y < card_y + card_h; y++) {
        if (y < 0 || y >= (int)height) continue;
        uint32_t row_off = y * width;
        for (int x = card_x; x < card_x + card_w; x++) {
            if (x < 0 || x >= (int)width) continue;
            if (is_outside_rounded_rect(x, y, card_x, card_y, card_w, card_h, card_r)) continue;

            bool is_edge = (x == card_x || x == card_x + card_w - 1 || y == card_y || y == card_y + card_h - 1 ||
                            is_outside_rounded_rect(x - 1, y, card_x, card_y, card_w, card_h, card_r) ||
                            is_outside_rounded_rect(x + 1, y, card_x, card_y, card_w, card_h, card_r) ||
                            is_outside_rounded_rect(x, y - 1, card_x, card_y, card_w, card_h, card_r) ||
                            is_outside_rounded_rect(x, y + 1, card_x, card_y, card_w, card_h, card_r));

            s_static_canvas[row_off + x] = is_edge ? card_border : card_bg;
        }
    }

    /* ATOMS Logo Chevron Mark */
    int logo_apex_x = cx;
    int logo_apex_y = card_y + 36;
    int logo_left_x = cx - 22;
    int logo_left_y = card_y + 70;
    int logo_right_x = cx + 22;
    int logo_right_y = card_y + 70;
    int stroke_thickness = 8;
    uint32_t logo_color = 0xFF38BDF8; // Light Blue

    draw_line_thick_round(s_static_canvas, width, height, width,
                          logo_apex_x, logo_apex_y, logo_left_x, logo_left_y,
                          stroke_thickness, logo_color);
    draw_line_thick_round(s_static_canvas, width, height, width,
                          logo_apex_x, logo_apex_y, logo_right_x, logo_right_y,
                          stroke_thickness, logo_color);

    /* Text */
    uint32_t white_color = 0xFFF1F5F9;
    uint32_t subtext_color = 0xFF94A3B8;

    draw_custom_text(s_static_canvas, width, height, width, cx, card_y + 92, "A T O M S", white_color, true);

    const char* status_text = s_is_restart_mode ? "Restarting..." : "Shutting down...";
    draw_custom_text(s_static_canvas, width, height, width, cx, card_y + 130, status_text, subtext_color, false);

    s_canvas_built = true;
}

static int shutdown_page_on_create(rook_page_t* page) {
    (void)page;
    s_shutdown_elapsed_ms = 0;
    s_canvas_built = false;
    AME_Spinner_Init(AME_GetBootSpinner(), 0, 0, 16, 10);
    s_shutdown_initialized = true;
    return 0;
}

static int shutdown_page_on_init(rook_page_t* page) {
    (void)page;
    s_shutdown_elapsed_ms = 0;
    s_canvas_built = false;
    return 0;
}

static int shutdown_page_on_load(rook_page_t* page) {
    (void)page;
    return 0;
}

static bool s_canvas_drawn_full = false;

static int shutdown_page_on_enter(rook_page_t* page) {
    (void)page;
    s_shutdown_elapsed_ms = 0;
    s_canvas_drawn_full = false;
    s_canvas_built = false;
    rook_invalidate_full();
    return 0;
}

static int shutdown_page_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    s_shutdown_elapsed_ms += delta_ms;
    AME_Update(delta_ms);
    rook_invalidate_full();
    return 0;
}

static int shutdown_page_on_render(rook_page_t* page, uint32_t* framebuffer, uint32_t stride) {
    (void)page;
    if (!framebuffer) return -1;

    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0 || height == 0) return -2;

    uint32_t stride_pixels = stride / 4;
    if (stride_pixels < width) stride_pixels = width;

    if (!s_canvas_built) {
        build_static_canvas(width, height);
    }

    int cx = (int)width / 2;
    int cy = (int)height / 2;

    int spinner_cx = cx;
    int spinner_cy = cy + 70;

    int spinner_rect_x = spinner_cx - 30;
    int spinner_rect_y = spinner_cy - 30;
    int spinner_rect_w = 60;
    int spinner_rect_h = 60;

    if (spinner_rect_x < 0) spinner_rect_x = 0;
    if (spinner_rect_y < 0) spinner_rect_y = 0;
    if (spinner_rect_x + spinner_rect_w > (int)width) spinner_rect_w = width - spinner_rect_x;
    if (spinner_rect_y + spinner_rect_h > (int)height) spinner_rect_h = height - spinner_rect_y;

    if (!s_canvas_drawn_full) {
        /* Initial full-screen canvas draw to backbuffer */
        for (uint32_t y = 0; y < height && y < 1080; y++) {
            uint32_t src_row = y * width;
            for (uint32_t x = 0; x < width && x < 1920; x++) {
                framebuffer[src_row + x] = s_static_canvas[src_row + x];
            }
        }
        s_canvas_drawn_full = true;
    } else {
        /* Restore static canvas background over spinner bounding box ONLY */
        for (int r = 0; r < spinner_rect_h; r++) {
            int py = spinner_rect_y + r;
            if (py < 0 || py >= (int)height) continue;
            uint32_t row_off = py * width + spinner_rect_x;
            for (int c = 0; c < spinner_rect_w; c++) {
                framebuffer[row_off + c] = s_static_canvas[row_off + c];
            }
        }
    }

    /* Render AME Spinner */
    AME_Spinner_SetPosition(AME_GetBootSpinner(), spinner_cx, spinner_cy);
    AME_Spinner_Render(AME_GetBootSpinner(), framebuffer, width, height, width);

    rook_invalidate_full();
    return 0;
}

static int shutdown_page_on_pause(rook_page_t* page) { (void)page; return 0; }
static int shutdown_page_on_resume(rook_page_t* page) { (void)page; return 0; }
static int shutdown_page_on_exit(rook_page_t* page) { (void)page; return 0; }
static int shutdown_page_on_unload(rook_page_t* page) { (void)page; return 0; }
static int shutdown_page_on_destroy(rook_page_t* page) { (void)page; return 0; }

rook_page_t* rook_page_shutdown_get(void) {
    if (!s_shutdown_initialized) {
        s_shutdown_page.id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.name = "ATOMS Shutdown";
        s_shutdown_page.state = ROOK_STATE_UNALLOCATED;

        s_shutdown_page.ops.on_create  = shutdown_page_on_create;
        s_shutdown_page.ops.on_init    = shutdown_page_on_init;
        s_shutdown_page.ops.on_load    = shutdown_page_on_load;
        s_shutdown_page.ops.on_enter   = shutdown_page_on_enter;
        s_shutdown_page.ops.on_update  = shutdown_page_on_update;
        s_shutdown_page.ops.on_render  = shutdown_page_on_render;
        s_shutdown_page.ops.on_pause   = shutdown_page_on_pause;
        s_shutdown_page.ops.on_resume  = shutdown_page_on_resume;
        s_shutdown_page.ops.on_exit    = shutdown_page_on_exit;
        s_shutdown_page.ops.on_unload  = shutdown_page_on_unload;
        s_shutdown_page.ops.on_destroy = shutdown_page_on_destroy;

        s_shutdown_page.nav_left_id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.nav_right_id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.nav_up_id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.nav_down_id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.nav_next_id = ROOK_PAGE_SHUTDOWN;
        s_shutdown_page.nav_prev_id = ROOK_PAGE_SHUTDOWN;
    }
    return &s_shutdown_page;
}
