#include "kernel/rook/include/rook.h"
#include "kernel/rook/include/rook_pages.h"
#include "kernel/lib/include/string.h"

/*
 * ♜ ROOK ENGINE V1.0 — Page 4: ATOMS OS Welcome Screen
 * Cinematic transition session preparation screen
 */

static uint32_t g_welcome_ticks = 0;

/* Access external wallpaper from page_login.c */
extern uint32_t* rook_get_wallpaper_buffer(void);
extern bool rook_is_wallpaper_loaded(void);

static inline void welcome_putpixel(uint32_t* fb, uint32_t w, uint32_t h, int32_t x, int32_t y, uint32_t color) {
    if (x >= 0 && x < (int32_t)w && y >= 0 && y < (int32_t)h) {
        fb[y * w + x] = color;
    }
}

static uint32_t welcome_fast_sqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = 1;
    while (x > y) {
        x = (x + y) >> 1;
        y = n / x;
    }
    return x;
}

static int32_t welcome_fast_atan2(int32_t y, int32_t x) {
    if (x == 0 && y == 0) return 0;
    int32_t abs_x = x < 0 ? -x : x;
    int32_t abs_y = y < 0 ? -y : y;
    int32_t angle;
    if (abs_x >= abs_y) {
        angle = (abs_y * 45) / abs_x;
    } else {
        angle = 90 - (abs_x * 45) / abs_y;
    }
    if (x < 0 && y >= 0) angle = 180 - angle;
    else if (x < 0 && y < 0) angle = 180 + angle;
    else if (x >= 0 && y < 0) angle = 360 - angle;
    return angle;
}

static inline uint32_t welcome_noise_2d(int32_t x, int32_t y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

/* Procedural galaxy drawing fallback with 25% dim overlay applied */
static void welcome_draw_galaxy_dim(uint32_t* fb, uint32_t width, uint32_t height) {
    int32_t gx = (width * 62) / 100;
    int32_t gy = (height * 46) / 100;

    for (uint32_t y = 0; y < height; y++) {
        uint32_t row_offset = y * width;
        int32_t dy = (int32_t)y - gy;
        for (uint32_t x = 0; x < width; x++) {
            int32_t dx = (int32_t)x - gx;

            int32_t rx = (dx * 86 - dy * 50) / 100;
            int32_t ry = (dx * 50 + dy * 86) / 100;

            uint32_t ell_dist = welcome_fast_sqrt((uint32_t)(rx*rx + ry*ry*10));
            uint32_t r_true = welcome_fast_sqrt((uint32_t)(rx*rx + ry*ry));

            uint8_t intensity = 0;
            if (ell_dist < 800) {
                uint32_t core_val = 0;
                if (ell_dist < 160) {
                    core_val = ((160 - ell_dist) * 0x75) / 160;
                }

                int32_t theta = welcome_fast_atan2(ry, rx);
                int32_t diff = (theta - (int32_t)(r_true * 7 / 5)) % 180;
                if (diff < 0) diff += 180;

                int32_t dist_to_arm = diff < 90 ? diff : 180 - diff;

                uint32_t arm_val = 0;
                if (ell_dist > 60 && ell_dist < 760) {
                    uint32_t base_arm = (90 - dist_to_arm) * (90 - dist_to_arm) / 80;
                    uint32_t radial_fade = ((760 - ell_dist) * 256) / 700;
                    arm_val = (base_arm * radial_fade) / 256;
                }

                uint32_t combined = core_val + arm_val;
                uint32_t n = welcome_noise_2d((int32_t)x, (int32_t)y) % 24;
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

            /* Apply 25% dim overlay (multiply by 0.75) */
            fb[row_offset + x] = 0xFF000000 | 
                                 (((r * 3 / 4) & 0xFF) << 16) | 
                                 (((g * 3 / 4) & 0xFF) << 8) | 
                                 ((b * 3 / 4) & 0xFF);
        }
    }

    /* Scatter 1,700 white/silver stars */
    uint32_t seed = 9999;
    for (uint32_t i = 0; i < 1700; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t sx = seed % width;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t sy = seed % height;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        uint32_t mag = seed % 16;

        uint32_t star_color = 0xFF1E1E1E;
        if (mag == 0) star_color = 0xFFBFBFBF;
        else if (mag < 3) star_color = 0xFF999999;
        else if (mag < 8) star_color = 0xFF4C4C4C;

        welcome_putpixel(fb, width, height, sx, sy, star_color);
    }
}

/* Font structure */
static const uint8_t welcome_font_data[128][8] = {
    ['A'] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00},
    ['M'] = {0x66, 0xFF, 0xDB, 0xDB, 0x66, 0x66, 0x66, 0x00},
    ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x63, 0x00},
    ['S'] = {0x3C, 0x66, 0x30, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['W'] = {0x66, 0x66, 0x66, 0xDB, 0xDB, 0xFF, 0x66, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void welcome_draw_char(uint32_t* fb, uint32_t w, uint32_t h, char c, int32_t x, int32_t y, int32_t scale, uint32_t color) {
    if ((uint8_t)c >= 128) return;
    const uint8_t* glyph = welcome_font_data[(uint8_t)c];
    for (int32_t row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int32_t col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                for (int32_t sy = 0; sy < scale; sy++) {
                    for (int32_t sx = 0; sx < scale; sx++) {
                        welcome_putpixel(fb, w, h, x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

static void draw_centered_str_welcome(uint32_t* fb, int fb_w, int fb_h, const char* str, int y, uint32_t color, int scale, int char_space) {
    int len = 0;
    while (str[len]) len++;
    int total_w = len * (8 * scale) + (len - 1) * char_space;
    int start_x = (fb_w - total_w) / 2;
    if (start_x < 0) start_x = 0;
    
    int cur_x = start_x;
    for (int i = 0; i < len; i++) {
        welcome_draw_char(fb, fb_w, fb_h, str[i], cur_x, y, scale, color);
        cur_x += (8 * scale) + char_space;
    }
}

static void welcome_fill_circle(uint32_t* fb, uint32_t w, uint32_t h, int32_t cx, int32_t cy, int32_t r, uint32_t color) {
    for (int32_t dy = -r; dy <= r; dy++) {
        for (int32_t dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                welcome_putpixel(fb, w, h, cx + dx, cy + dy, color);
            }
        }
    }
}

/* Atom Logo Orbit Drawer */
static void welcome_draw_atom_logo(uint32_t* fb, uint32_t w, uint32_t h, int32_t cx, int32_t cy, int32_t rx, int32_t ry, int32_t core_r, int32_t electron_offset) {
    uint32_t white = 0xFFFFFFFF;
    welcome_fill_circle(fb, w, h, cx, cy, core_r, white);

    /* Ring 1: Horizontal */
    for (int32_t t = -rx; t <= rx; t++) {
        int32_t dy = (ry * (rx - t) * (rx + t)) / (rx * rx);
        if (dy >= 0) {
            int32_t s = 0;
            while (s * s <= dy * ry) s++;
            if (s > 0) s--;
            welcome_putpixel(fb, w, h, cx + t, cy + s, white);
            welcome_putpixel(fb, w, h, cx + t, cy - s, white);
        }
    }

    /* Ring 2 & Ring 3: Diagonal tilted rings */
    for (int32_t t = -rx; t <= rx; t += 2) {
        int32_t diag_y = t / 2;
        int32_t width_x = rx - (t * t) / rx;
        if (width_x > 0) {
            int32_t s = 0;
            while (s * s <= width_x * 12) s++;
            if (s > 0) s--;
            welcome_putpixel(fb, w, h, cx + t/2 + s, cy - diag_y + s/2, white);
            welcome_putpixel(fb, w, h, cx + t/2 - s, cy - diag_y - s/2, white);
            welcome_putpixel(fb, w, h, cx - t/2 + s, cy - diag_y - s/2, white);
            welcome_putpixel(fb, w, h, cx - t/2 - s, cy - diag_y + s/2, white);
        }
    }

    /* 3 Electrons */
    welcome_fill_circle(fb, w, h, cx + electron_offset, cy, 4, white);
    welcome_fill_circle(fb, w, h, cx - (electron_offset/2), cy - (electron_offset*3/5), 4, white);
    welcome_fill_circle(fb, w, h, cx - (electron_offset/2), cy + (electron_offset*3/5), 4, white);
}

static int welcome_on_create(rook_page_t* page) {
    page->name = "ATOMS OS Welcome Screen";
    page->nav_next_id = ROOK_PAGE_DESKTOP;
    return 0;
}

static int welcome_on_enter(rook_page_t* page) {
    (void)page;
    g_welcome_ticks = 0;
    return 0;
}

static int welcome_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    g_welcome_ticks += (uint32_t)delta_ms;
    return 0;
}

static int welcome_on_render(rook_page_t* page, uint32_t* fb, uint32_t stride) {
    (void)page;
    (void)stride;
    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();

    /* 1. Apply wallpaper with 25% dim overlay */
    uint32_t* wall_buf = rook_get_wallpaper_buffer();
    if (rook_is_wallpaper_loaded() && wall_buf != NULL) {
        uint32_t total = width * height;
        for (uint32_t i = 0; i < total; i++) {
            uint32_t pixel = wall_buf[i];
            uint32_t r = ((pixel >> 16) & 0xFF) * 3 / 4;
            uint32_t g = ((pixel >> 8) & 0xFF) * 3 / 4;
            uint32_t b = (pixel & 0xFF) * 3 / 4;
            fb[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    } else {
        welcome_draw_galaxy_dim(fb, width, height);
    }

    int center_x = width / 2;
    int center_y = height / 2;

    /* 2. Atom Logo at center */
    welcome_draw_atom_logo(fb, width, height, center_x, center_y - 120, 56, 18, 10, 50);

    /* 3. Text WELCOME & Profile */
    draw_centered_str_welcome(fb, width, height, "WELCOME", center_y - 40, 0xFFFFFFFF, 3, 16);
    draw_centered_str_welcome(fb, width, height, "ADMINISTRATOR", center_y + 15, 0xFFBBBBBB, 2, 8);

    /* 4. Dynamic status messages tied to real boot milestones */
    const char* status_msg = "PREPARING YOUR WORKSPACE...";
    if (g_welcome_ticks > 500 && g_welcome_ticks <= 1000) {
        status_msg = "LOADING USER PROFILE...";
    } else if (g_welcome_ticks > 1000 && g_welcome_ticks <= 1500) {
        status_msg = "LOADING DESKTOP...";
    } else if (g_welcome_ticks > 1500 && g_welcome_ticks <= 2000) {
        status_msg = "STARTING EXPLORER...";
    } else if (g_welcome_ticks > 2000 && g_welcome_ticks <= 2300) {
        status_msg = "LOADING SERVICES...";
    } else if (g_welcome_ticks > 2300) {
        status_msg = "ALMOST READY...";
    }
    draw_centered_str_welcome(fb, width, height, status_msg, center_y + 55, 0xFF888888, 1, 4);

    /* 5. Cycling dot progress indicator below */
    int cycle = (g_welcome_ticks / 250) % 3;
    int dot_y = center_y + 90;
    
    /* Draw 3 spaced dot spheres */
    welcome_fill_circle(fb, width, height, center_x - 30, dot_y, 4, (cycle == 0) ? 0xFFFFFFFF : 0xFF444444);
    welcome_fill_circle(fb, width, height, center_x,      dot_y, 4, (cycle == 1) ? 0xFFFFFFFF : 0xFF444444);
    welcome_fill_circle(fb, width, height, center_x + 30, dot_y, 4, (cycle == 2) ? 0xFFFFFFFF : 0xFF444444);

    /* 6. Cinematic 300 ms fade-to-black at the end of Welcome lifetime (from 2200 ms to 2500 ms) */
    if (g_welcome_ticks > 2200) {
        uint32_t elapsed = g_welcome_ticks - 2200;
        if (elapsed > 300) elapsed = 300;
        uint32_t fade_factor = 255 - (elapsed * 255 / 300);

        uint32_t total = width * height;
        for (uint32_t i = 0; i < total; i++) {
            uint32_t pixel = fb[i];
            uint32_t r = (((pixel >> 16) & 0xFF) * fade_factor) / 255;
            uint32_t g = (((pixel >> 8) & 0xFF) * fade_factor) / 255;
            uint32_t b = ((pixel & 0xFF) * fade_factor) / 255;
            fb[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    return 0;
}

static rook_page_t g_page_welcome = {
    .id = ROOK_PAGE_WELCOME,
    .state = ROOK_STATE_UNALLOCATED,
    .ops = {
        .on_create = welcome_on_create,
        .on_enter  = welcome_on_enter,
        .on_update = welcome_on_update,
        .on_render = welcome_on_render
    }
};

rook_page_t* rook_page_welcome_get(void) {
    return &g_page_welcome;
}
