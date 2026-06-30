#include "kernel/rook/include/rook.h"
#include "kernel/rook/pages/atom_logo.h"
#include "kernel/bofont/bofont.h"

/*
 * ♜ ROOK ENGINE V1.0 — Page 0: ATOMS OS Boot Splash Screen
 * Features:
 * - Pure Pitch Black background (#000000)
 * - Crisp, flat white Atom Logo (⚛) without glow/blur shaders
 * - Modern wide-spaced typography ("A T O M S   O S")
 * - Event-driven growing dot progress animation (.. -> .... -> ......)
 */

static uint32_t g_boot_progress_dots = 2; /* Starts at 2 dots: ".." */

/* Helper: Put pixel on backbuffer with bounds check */
static inline void boot_putpixel(uint32_t* fb, uint32_t w, uint32_t h, uint32_t stride, int32_t x, int32_t y, uint32_t color) {
    if (x >= 0 && (uint32_t)x < w && y >= 0 && (uint32_t)y < h) {
        fb[y * (stride / 4) + x] = color;
    }
}

/* Helper: Draw crisp filled circle (Atom Nucleus / Electrons / Dots) */
static void boot_fill_circle(uint32_t* fb, uint32_t w, uint32_t h, uint32_t stride, int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    for (int32_t y = -r; y <= r; y++) {
        for (int32_t x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                boot_putpixel(fb, w, h, stride, cx + x, cy + y, color);
            }
        }
    }
}

/* Helper: Draw procedural Atom Logo ellipses using integer math */
static void boot_draw_atom_logo(uint32_t* fb, uint32_t w, uint32_t h, uint32_t stride, int32_t cx, int32_t cy) {
    uint32_t white = 0xFFFFFFFF;
    
    /* 1. Draw central nucleus sphere */
    boot_fill_circle(fb, w, h, stride, cx, cy, 12, white);

    /* 2. Draw 3 orbiting rings (0 deg horizontal, 60 deg tilted, 120 deg tilted) */
    /* Using high-precision integer parametric loop for crisp 2px rings */
    for (int32_t angle = 0; angle < 360; angle++) {
        /* Approximate trig table or integer ellipse equation */
        /* For 0 deg ring: x = rx * cos(t), y = ry * sin(t) */
        /* Simple integer ellipse rasterization for 3 distinct orbits */
        int32_t rx = 64, ry = 22;
        
        /* Ring 1: Horizontal */
        for (int32_t t = -rx; t <= rx; t++) {
            int32_t dy = (ry * (rx - t) * (rx + t)) / (rx * rx);
            if (dy >= 0) {
                /* Approximate square root via integer iterations */
                int32_t s = 0;
                while (s * s <= dy * ry) s++;
                if (s > 0) s--;
                boot_putpixel(fb, w, h, stride, cx + t, cy + s, white);
                boot_putpixel(fb, w, h, stride, cx + t, cy - s, white);
            }
        }

        /* Ring 2 & Ring 3: Diagonal tilted rings (+60° and -60°) */
        for (int32_t t = -rx; t <= rx; t += 2) {
            int32_t diag_y = t / 2;
            int32_t width_x = rx - (t * t) / rx;
            if (width_x > 0) {
                int32_t s = 0;
                while (s * s <= width_x * 12) s++;
                if (s > 0) s--;
                /* Ring 2 (/ tilt) */
                boot_putpixel(fb, w, h, stride, cx + t/2 + s, cy - diag_y + s/2, white);
                boot_putpixel(fb, w, h, stride, cx + t/2 - s, cy - diag_y - s/2, white);
                /* Ring 3 (\ tilt) */
                boot_putpixel(fb, w, h, stride, cx - t/2 + s, cy - diag_y - s/2, white);
                boot_putpixel(fb, w, h, stride, cx - t/2 - s, cy - diag_y + s/2, white);
            }
        }
    }

    /* 3. Draw 3 crisp electron spheres along the orbits */
    boot_fill_circle(fb, w, h, stride, cx + 58, cy, 5, white);
    boot_fill_circle(fb, w, h, stride, cx - 28, cy - 35, 5, white);
    boot_fill_circle(fb, w, h, stride, cx - 28, cy + 35, 5, white);
}

/* Built-in 8x8 font lookup for early boot screen typography */
static const uint8_t boot_font_data[128][8] = {
    ['A'] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['M'] = {0x66, 0xFF, 0xDB, 0xDB, 0x66, 0x66, 0x66, 0x00},
    ['S'] = {0x3C, 0x66, 0x30, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
    ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x63, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['F'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void boot_draw_char_scaled(uint32_t* fb, uint32_t w, uint32_t h, uint32_t stride, char c, int32_t x, int32_t y, int32_t scale, uint32_t color) {
    if ((uint8_t)c >= 128) return;
    const uint8_t* glyph = boot_font_data[(uint8_t)c];
    for (int32_t row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int32_t col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                for (int32_t sy = 0; sy < scale; sy++) {
                    for (int32_t sx = 0; sx < scale; sx++) {
                        boot_putpixel(fb, w, h, stride, x + col*scale + sx, y + row*scale + sy, color);
                    }
                }
            }
        }
    }
}

static void boot_draw_string_spaced(uint32_t* fb, uint32_t w, uint32_t h, uint32_t stride, const char* str, int32_t center_x, int32_t y, int32_t scale, int32_t char_spacing) {
    /* Calculate total string width for exact centering */
    int32_t len = 0;
    for (const char* p = str; *p; p++) len++;
    int32_t total_width = len * (8 * scale) + (len - 1) * char_spacing;
    int32_t start_x = center_x - (total_width / 2);

    int32_t cur_x = start_x;
    for (const char* p = str; *p; p++) {
        boot_draw_char_scaled(fb, w, h, stride, *p, cur_x, y, scale, 0xFFFFFFFF);
        cur_x += (8 * scale) + char_spacing;
    }
}

/* 11-Stage Lifecycle Implementations */
static int boot_on_create(rook_page_t* page) {
    page->name = "ATOMS OS Boot Splash";
    page->nav_next_id = ROOK_PAGE_LOGIN;
    return 0;
}

static int boot_on_enter(rook_page_t* page) {
    (void)page;
    g_boot_progress_dots = 2; /* Reset dots to ".." */
    rook_invalidate_full();
    return 0;
}

static int boot_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    /* Event-driven progression */
    if (delta_ms >= ROOK_EVENT_MEM_READY) {
        if (g_boot_progress_dots < 14) {
            g_boot_progress_dots += 2;
            rook_invalidate_full();
        }
    } else {
        /* Automatic dot tick fallback during idle wait */
        static uint64_t accum = 0;
        accum += delta_ms;
        if (accum > 300 && g_boot_progress_dots < 14) {
            g_boot_progress_dots += 2;
            accum = 0;
            rook_invalidate_full();
        }
    }
    return 0;
}

static int boot_on_render(rook_page_t* page, uint32_t* fb, uint32_t stride) {
    (void)page;
    uint32_t w = rook_get_width();
    uint32_t h = rook_get_height();

    /* 1. Clear canvas to solid Pitch Black (#000000) */
    uint32_t pitch_pixels = stride / 4;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            fb[y * pitch_pixels + x] = 0x00000000;
        }
    }

    int32_t cx = w / 2;
    int32_t cy = (h / 2) - 60;

    /* 2. Draw sharp white Atom Logo (⚛) */
    boot_draw_atom_logo(fb, w, h, stride, cx, cy);

    /* 3. Draw spaced typography below logo */
    boot_draw_string_spaced(fb, w, h, stride, "ATOMS OS", cx, cy + 90, 3, 16);
    boot_draw_string_spaced(fb, w, h, stride, "ENGINEERED FOR THE FUTURE", cx, cy + 135, 1, 8);

    /* 4. Draw growing dot progress indicator below typography */
    int32_t dot_start_x = cx - ((g_boot_progress_dots * 16) / 2);
    for (uint32_t i = 0; i < g_boot_progress_dots; i++) {
        boot_fill_circle(fb, w, h, stride, dot_start_x + (i * 16), cy + 185, 3, 0xFFFFFFFF);
    }

    return 0;
}

/* Page Descriptor Instance */
static rook_page_t g_page_boot = {
    .id = ROOK_PAGE_BOOT_SPLASH,
    .state = ROOK_STATE_UNALLOCATED,
    .ops = {
        .on_create = boot_on_create,
        .on_enter  = boot_on_enter,
        .on_update = boot_on_update,
        .on_render = boot_on_render
    }
};

rook_page_t* rook_page_boot_get(void) {
    return &g_page_boot;
}
