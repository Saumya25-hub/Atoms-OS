#include "kernel/shell/rook/pages/page_boot.h"
#include "kernel/shell/rook/include/spinner.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/ame/include/ame.h"
#include "kernel/core/lib/include/string.h"
#include "bovisual/Include/graphics.h"
#include "bovisual/Include/text.h"
#include "kernel/ui/bofont/bofont.h"

/*
 * ♜ ATOMS OS Boot Splash Page (Page 0: ROOK_PAGE_BOOT_SPLASH)
 * Phase 1 Production Boot Splash — Driven by ATOMS Motion Engine (AME)
 * Pure black canvas (#000000)
 * Pre-rendered static cache: ATOMS logo mark, "ATOMS" title, "OPERATING SYSTEM" subtext
 * Fluid Windows 11 / Linux loading dynamics via AME Spinner Module
 * Zero heap allocations, zero flicker, 100% tear-free single-present.
 */

static rook_page_t s_boot_page;
static uint64_t    s_boot_elapsed_ms = 0;
static bool        s_boot_initialized = false;

/* Offscreen static canvas cache (up to 1920x1080) */
static uint32_t    s_static_canvas[1920 * 1080] __attribute__((aligned(16)));
static bool        s_canvas_built = false;

/* High-precision line rendering with round caps for ATOMS chevron logo mark */
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

/* Vector/BOFont character renderer */
static void draw_custom_text(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride_pixels, int cx, int y, const char* str, uint32_t color, bool large) {
    if (!str) return;

    int len = 0;
    while (str[len]) len++;

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

    int text_x = cx - (len * 8) / 2;
    BOVISUAL_Draw_String(text_x, y, str, color, 0x00000000, true, NULL);
}

/* Pre-render static black canvas with Logo, Title, and Subtext */
static void build_static_canvas(uint32_t width, uint32_t height, uint32_t stride_pixels) {
    uint32_t total_pixels = width * height;
    for (uint32_t i = 0; i < total_pixels; i++) {
        s_static_canvas[i] = 0x00000000;
    }

    int cx = (int)width / 2;
    int cy = (int)height / 2;

    int logo_apex_x = cx;
    int logo_apex_y = cy - 85;
    int logo_left_x = cx - 28;
    int logo_left_y = cy - 38;
    int logo_right_x = cx + 28;
    int logo_right_y = cy - 38;
    int stroke_thickness = 11;
    uint32_t white_color = 0x00FFFFFF;
    uint32_t gray_color = 0x00888888;

    draw_line_thick_round(s_static_canvas, width, height, stride_pixels,
                          logo_apex_x, logo_apex_y, logo_left_x, logo_left_y,
                          stroke_thickness, white_color);
    draw_line_thick_round(s_static_canvas, width, height, stride_pixels,
                          logo_apex_x, logo_apex_y, logo_right_x, logo_right_y,
                          stroke_thickness, white_color);

    draw_custom_text(s_static_canvas, width, height, stride_pixels, cx, cy + 5, "A T O M S", white_color, true);
    draw_custom_text(s_static_canvas, width, height, stride_pixels, cx, cy + 38, "OPERATING SYSTEM", gray_color, false);

    s_canvas_built = true;
}

static int boot_page_on_create(rook_page_t* page) {
    (void)page;
    s_boot_elapsed_ms = 0;
    s_canvas_built = false;
    AME_Spinner_Init(AME_GetBootSpinner(), 0, 0, 18, 12);
    s_boot_initialized = true;
    return 0;
}

static int boot_page_on_init(rook_page_t* page) {
    (void)page;
    s_boot_elapsed_ms = 0;
    s_canvas_built = false;
    return 0;
}

static int boot_page_on_load(rook_page_t* page) {
    (void)page;
    return 0;
}

static int boot_page_on_enter(rook_page_t* page) {
    (void)page;
    s_boot_elapsed_ms = 0;
    return 0;
}

static int boot_page_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    s_boot_elapsed_ms += delta_ms;
    AME_Update(delta_ms);
    return 0;
}

static int boot_page_on_render(rook_page_t* page, uint32_t* framebuffer, uint32_t stride) {
    (void)page;
    if (!framebuffer) return -1;

    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    if (width == 0 || height == 0) return -2;

    uint32_t stride_pixels = stride / 4;
    if (stride_pixels == 0) stride_pixels = width;

    /* Build static layer once */
    if (!s_canvas_built) {
        build_static_canvas(width, height, stride_pixels);
    }

    int cx = (int)width / 2;
    int cy = (int)height / 2;

    int spinner_rect_x = cx - 35;
    int spinner_rect_y = cy + 65;
    int spinner_rect_w = 70;
    int spinner_rect_h = 60;

    if (spinner_rect_x < 0) spinner_rect_x = 0;
    if (spinner_rect_y < 0) spinner_rect_y = 0;
    if (spinner_rect_x + spinner_rect_w > (int)width) spinner_rect_w = width - spinner_rect_x;
    if (spinner_rect_y + spinner_rect_h > (int)height) spinner_rect_h = height - spinner_rect_y;

    /* Restore static canvas background over spinner bounding box */
    for (int r = 0; r < spinner_rect_h; r++) {
        uint32_t offset = (spinner_rect_y + r) * stride_pixels + spinner_rect_x;
        for (int c = 0; c < spinner_rect_w; c++) {
            framebuffer[offset + c] = s_static_canvas[offset + c];
        }
    }

    /* If first frame or canvas reset, copy full canvas to framebuffer */
    static bool s_first_frame = true;
    if (s_first_frame) {
        uint32_t total = width * height;
        for (uint32_t i = 0; i < total; i++) {
            framebuffer[i] = s_static_canvas[i];
        }
        s_first_frame = false;
        rook_invalidate_full();
    } else {
        /* Dirty region update for spinner region only */
        rook_invalidate_rect(spinner_rect_x, spinner_rect_y, spinner_rect_w, spinner_rect_h);
    }

    /* Render AME System Spinner on offscreen framebuffer */
    AME_Spinner_SetPosition(AME_GetBootSpinner(), cx, cy + 95);
    AME_Spinner_Render(AME_GetBootSpinner(), framebuffer, width, height, stride);

    return 0;
}

static int boot_page_on_pause(rook_page_t* page) { (void)page; return 0; }
static int boot_page_on_resume(rook_page_t* page) { (void)page; return 0; }
static int boot_page_on_exit(rook_page_t* page) { (void)page; return 0; }
static int boot_page_on_unload(rook_page_t* page) { (void)page; return 0; }
static int boot_page_on_destroy(rook_page_t* page) { (void)page; return 0; }

rook_page_t* rook_page_boot_get(void) {
    if (!s_boot_initialized) {
        s_boot_page.id = ROOK_PAGE_BOOT_SPLASH;
        s_boot_page.name = "ATOMS Boot Splash";
        s_boot_page.state = ROOK_STATE_UNALLOCATED;

        s_boot_page.ops.on_create  = boot_page_on_create;
        s_boot_page.ops.on_init    = boot_page_on_init;
        s_boot_page.ops.on_load    = boot_page_on_load;
        s_boot_page.ops.on_enter   = boot_page_on_enter;
        s_boot_page.ops.on_update  = boot_page_on_update;
        s_boot_page.ops.on_render  = boot_page_on_render;
        s_boot_page.ops.on_pause   = boot_page_on_pause;
        s_boot_page.ops.on_resume  = boot_page_on_resume;
        s_boot_page.ops.on_exit    = boot_page_on_exit;
        s_boot_page.ops.on_unload  = boot_page_on_unload;
        s_boot_page.ops.on_destroy = boot_page_on_destroy;

        s_boot_page.nav_left_id = ROOK_PAGE_BOOT_SPLASH;
        s_boot_page.nav_right_id = ROOK_PAGE_BOOT_SPLASH;
        s_boot_page.nav_up_id = ROOK_PAGE_BOOT_SPLASH;
        s_boot_page.nav_down_id = ROOK_PAGE_BOOT_SPLASH;
        s_boot_page.nav_next_id = ROOK_PAGE_DESKTOP;
        s_boot_page.nav_prev_id = ROOK_PAGE_BOOT_SPLASH;
    }
    return &s_boot_page;
}
