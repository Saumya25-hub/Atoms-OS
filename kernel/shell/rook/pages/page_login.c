#include "kernel/shell/rook/include/rook.h"
#include "kernel/shell/rook/include/rook_pages.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/identity/include/identity.h"
#include "kernel/ame/include/ame.h"
#include "bovisual/Include/events.h"

/*
 * ♜ ROOK ENGINE V1.0 — Page 3: ATOMS OS Login Screen
 * High-Fidelity Hybrid Login Screen: Interactive Authentication with AME Shake Animation & Fallback Galaxy
 */

/* State variables */
static uint32_t g_login_ticks = 0;
static char s_password_buf[64] = "";
static int s_password_len = 0;
static bool s_error_state = false;
static AME_Handle s_shake_handle = AME_INVALID_HANDLE;

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

/* Fast integer atan2 approximation in degrees (0 to 359) */
static int32_t fast_atan2(int32_t y, int32_t x) {
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

/* 2D Hash Noise Generator for organic gas dust texture */
static inline uint32_t noise_2d(int32_t x, int32_t y) {
    uint32_t h = (uint32_t)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

/* Background wallpaper load step (called during boot splash loader to prevent frame stutter) */
void login_wallpaper_load_step(void) {
    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();
    uint32_t total_bytes = width * height * 4;

    if (g_wallpaper_loaded) return;

    /* 1. Allocate physical pages for the wallpaper once */
    if (g_wallpaper_buffer == NULL) {
        uint32_t pages = (total_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
        g_wallpaper_buffer = (uint32_t*)pmm_alloc_pages(pages);
        if (g_wallpaper_buffer == NULL) {
            g_wallpaper_loaded = true;
            return;
        }
    }

    /* 2. Open the file if not already open */
    if (g_wallpaper_fd < 0) {
        g_wallpaper_fd = vfs_open("/BOOT.RAW");
        if (g_wallpaper_fd < 0) {
            g_wallpaper_loaded = true;
            return;
        }
        g_wallpaper_bytes_loaded = 0;
    }

    /* 3. Read a chunk of 64KB per frame to keep boot splash animation 100% smooth */
    if (g_wallpaper_bytes_loaded < total_bytes) {
        uint32_t chunk_size = total_bytes - g_wallpaper_bytes_loaded;
        if (chunk_size > 65536) chunk_size = 65536;

        uint8_t* dest = (uint8_t*)g_wallpaper_buffer + g_wallpaper_bytes_loaded;
        int read_len = vfs_read(g_wallpaper_fd, dest, chunk_size);
        if (read_len <= 0) {
            vfs_close(g_wallpaper_fd);
            g_wallpaper_fd = -1;
            g_wallpaper_loaded = true;
            return;
        }
        g_wallpaper_bytes_loaded += read_len;
    }

    /* 4. Complete loading when bytes match */
    if (g_wallpaper_bytes_loaded >= total_bytes) {
        if (g_wallpaper_fd >= 0) {
            vfs_close(g_wallpaper_fd);
            g_wallpaper_fd = -1;
        }
        g_wallpaper_loaded = true;
    }
}

/* Built-in font data for Login Screen */
static const uint8_t login_font_data[128][8] = {
    ['A'] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['B'] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
    ['F'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['K'] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00},
    ['L'] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00},
    ['M'] = {0x66, 0xFF, 0xDB, 0xDB, 0x66, 0x66, 0x66, 0x00},
    ['N'] = {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['R'] = {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x63, 0x00},
    ['S'] = {0x3C, 0x66, 0x30, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
    ['W'] = {0x66, 0x66, 0x66, 0xDB, 0xDB, 0xFF, 0x66, 0x00},
    ['X'] = {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    ['0'] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['3'] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['5'] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00},
    ['*'] = {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
    ['['] = {0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00},
    [']'] = {0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00},
    ['-'] = {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    ['>'] = {0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void login_draw_char(uint32_t* fb, uint32_t w, uint32_t h, char c, int32_t x, int32_t y, int32_t scale, uint32_t color) {
    if ((uint8_t)c >= 128) return;
    const uint8_t* glyph = login_font_data[(uint8_t)c];
    for (int32_t row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int32_t col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                for (int32_t sy = 0; sy < scale; sy++) {
                    for (int32_t sx = 0; sx < scale; sx++) {
                        login_putpixel(fb, w, h, x + col * scale + sx, y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

static void draw_centered_str(uint32_t* fb, int fb_w, int fb_h, const char* str, int y, uint32_t color, int scale, int char_space, int x_offset) {
    int len = 0;
    while (str[len]) len++;
    int total_w = len * (8 * scale) + (len - 1) * char_space;
    int start_x = (fb_w - total_w) / 2 + x_offset;
    if (start_x < 0) start_x = 0;
    
    int cur_x = start_x;
    for (int i = 0; i < len; i++) {
        login_draw_char(fb, fb_w, fb_h, str[i], cur_x, y, scale, color);
        cur_x += (8 * scale) + char_space;
    }
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

/* Exact Boot Splash Atom Logo for Consistency */
static void login_draw_atom_logo(uint32_t* fb, uint32_t w, uint32_t h, int32_t cx, int32_t cy, int32_t rx, int32_t ry, int32_t core_r, int32_t electron_offset) {
    uint32_t white = 0xFFFFFFFF;
    login_fill_circle(fb, w, h, cx, cy, core_r, white);

    /* Ring 1: Horizontal */
    for (int32_t t = -rx; t <= rx; t++) {
        int32_t dy = (ry * (rx - t) * (rx + t)) / (rx * rx);
        if (dy >= 0) {
            int32_t s = 0;
            while (s * s <= dy * ry) s++;
            if (s > 0) s--;
            login_putpixel(fb, w, h, cx + t, cy + s, white);
            login_putpixel(fb, w, h, cx + t, cy - s, white);
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
            login_putpixel(fb, w, h, cx + t/2 + s, cy - diag_y + s/2, white);
            login_putpixel(fb, w, h, cx + t/2 - s, cy - diag_y - s/2, white);
            login_putpixel(fb, w, h, cx - t/2 + s, cy - diag_y - s/2, white);
            login_putpixel(fb, w, h, cx - t/2 - s, cy - diag_y + s/2, white);
        }
    }

    /* 3 Electrons */
    login_fill_circle(fb, w, h, cx + electron_offset, cy, 4, white);
    login_fill_circle(fb, w, h, cx - (electron_offset/2), cy - (electron_offset*3/5), 4, white);
    login_fill_circle(fb, w, h, cx - (electron_offset/2), cy + (electron_offset*3/5), 4, white);
}

/* Highly textured, photorealistic spiral galaxy swirl with warm core (exactly like Screenshot 2) */
static void draw_visible_spiral_galaxy(uint32_t* fb, uint32_t width, uint32_t height) {
    int32_t gx = (width * 62) / 100;
    int32_t gy = (height * 46) / 100;

    for (uint32_t y = 0; y < height; y++) {
        uint32_t row_offset = y * width;
        int32_t dy = (int32_t)y - gy;
        for (uint32_t x = 0; x < width; x++) {
            int32_t dx = (int32_t)x - gx;

            /* Rotation matrix for diagonal sweep (approx -30 degrees) */
            int32_t rx = (dx * 86 - dy * 50) / 100;
            int32_t ry = (dx * 50 + dy * 86) / 100;

            /* Flat disc elliptical projection */
            uint32_t ell_dist = fast_sqrt((uint32_t)(rx*rx + ry*ry*10));

            /* True radius for arm wrapping */
            uint32_t r_true = fast_sqrt((uint32_t)(rx*rx + ry*ry));

            uint8_t intensity = 0;
            if (ell_dist < 800) {
                /* Core brightness */
                uint32_t core_val = 0;
                if (ell_dist < 160) {
                    core_val = ((160 - ell_dist) * 0x75) / 160;
                }

                /* Wrap tightly to get clean multiple spiral arm lanes */
                int32_t theta = fast_atan2(ry, rx);
                int32_t diff = (theta - (int32_t)(r_true * 7 / 5)) % 180;
                if (diff < 0) diff += 180;

                int32_t dist_to_arm = diff < 90 ? diff : 180 - diff;

                /* Continuous arm rendering with smooth falloff */
                uint32_t arm_val = 0;
                if (ell_dist > 60 && ell_dist < 760) {
                    uint32_t base_arm = (90 - dist_to_arm) * (90 - dist_to_arm) / 80;
                    uint32_t radial_fade = ((760 - ell_dist) * 256) / 700;
                    arm_val = (base_arm * radial_fade) / 256;
                }

                uint32_t combined = core_val + arm_val;

                /* Add high-frequency organic stardust texture/granularity */
                uint32_t n = noise_2d((int32_t)x, (int32_t)y) % 24;
                if (combined > 4) {
                    combined = combined - 4 + (n * combined / 90);
                }

                intensity = (uint8_t)(combined <= 255 ? combined : 255);
            }

            /* Map colors: Warm Amber Core (like 2nd photo), Cool Silver Arms */
            uint8_t r = intensity, g = intensity, b = intensity;
            if (ell_dist < 260) {
                uint32_t t = 260 - ell_dist;
                /* Add warm amber hue components (ratio: R:1.15, G:0.85, B:0.55) */
                r = (intensity * (100 + (35 * t / 260))) / 100;
                g = (intensity * (100 + (5 * t / 260))) / 100;
                b = (intensity * (100 - (35 * t / 260))) / 100;
            }

            fb[row_offset + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
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

        uint32_t star_color = 0xFF282828;
        if (mag == 0) star_color = 0xFFFFFFFF;
        else if (mag < 3) star_color = 0xFFCCCCCC;
        else if (mag < 8) star_color = 0xFF666666;

        login_putpixel(fb, width, height, sx, sy, star_color);

        /* Sparkles for top magnitude stars */
        if (mag == 0) {
            login_putpixel(fb, width, height, sx - 1, sy, 0xFF999999);
            login_putpixel(fb, width, height, sx + 1, sy, 0xFF999999);
            login_putpixel(fb, width, height, sx, sy - 1, 0xFF999999);
            login_putpixel(fb, width, height, sx, sy + 1, 0xFF999999);
        }
    }
}

static void login_attempt_auth(void) {
    extern void Desktop_Shell_StartBootExperience(void);
    
    bool success = Identity_Authenticate("admin", s_password_buf);
    if (success) {
        s_error_state = false;
        s_password_len = 0;
        s_password_buf[0] = '\0';
        Desktop_Shell_StartBootExperience();
    } else {
        s_error_state = true;
        s_password_len = 0;
        s_password_buf[0] = '\0';
        
        if (s_shake_handle != AME_INVALID_HANDLE) {
            AME_DestroyAnimation(s_shake_handle);
        }
        // Trigger AME horizontal elastic shake on failure
        s_shake_handle = AME_CreateAnimation(NULL, PROP_X, 24, 0, 500, EASE_OUT_ELASTIC);
        AME_Play(s_shake_handle);
    }
}

void page_login_handle_event(const BVEvent* ev) {
    if (!ev) return;

    if (ev->type == BV_EVENT_KEY_DOWN) {
        if (s_error_state) {
            s_error_state = false;
        }

        // Backspace
        if (ev->key_code == 0x0E || ev->ascii == 8 || ev->ascii == '\b' || ev->key_code == 0x08) {
            if (s_password_len > 0) {
                s_password_len--;
                s_password_buf[s_password_len] = '\0';
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
            }
        }
    } else if (ev->type == BV_EVENT_MOUSE_DOWN) {
        uint32_t width = rook_get_width();
        uint32_t height = rook_get_height();
        int center_x = width / 2;
        int card_y = 205;

        // LOGIN button bounds
        int btn_x = center_x - 80;
        int btn_y = card_y + 250;
        int btn_w = 160;
        int btn_h = 36;

        if (ev->mouse_x >= btn_x && ev->mouse_x <= btn_x + btn_w &&
            ev->mouse_y >= btn_y && ev->mouse_y <= btn_y + btn_h) {
            login_attempt_auth();
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
    if (s_shake_handle != AME_INVALID_HANDLE) {
        AME_DestroyAnimation(s_shake_handle);
        s_shake_handle = AME_INVALID_HANDLE;
    }
    return 0;
}

static int login_on_update(rook_page_t* page, uint64_t delta_ms) {
    (void)page;
    g_login_ticks += (uint32_t)delta_ms;
    return 0;
}

static int login_on_render(rook_page_t* page, uint32_t* fb, uint32_t stride) {
    (void)page;
    (void)stride;
    uint32_t width = rook_get_width();
    uint32_t height = rook_get_height();

    /* Instantly draw the pre-loaded wallpaper from memory buffer */
    if (g_wallpaper_loaded && g_wallpaper_buffer != NULL) {
        uint32_t total_pixels = width * height;
        for (uint32_t i = 0; i < total_pixels; i++) {
            fb[i] = g_wallpaper_buffer[i];
        }
    } else {
        /* Fallback: Photorealistic Diagonal Amber-Silver Spiral Galaxy */
        draw_visible_spiral_galaxy(fb, width, height);
    }

    int center_x = width / 2;
    int center_y = height / 2;

    /* 2. Top Header — Exact Boot Atom Logo */
    login_draw_atom_logo(fb, width, height, center_x, 80, 56, 18, 10, 50);
    draw_centered_str(fb, width, height, "ATOMS OS", 130, 0xFFFFFFFF, 3, 16, 0);
    draw_centered_str(fb, width, height, "ENGINEERED FOR THE FUTURE", 165, 0xFF888888, 1, 6, 0);

    /* Calculate AME horizontal shake offset */
    int shake_offset = 0;
    if (s_shake_handle != AME_INVALID_HANDLE) {
        if (AME_GetState(s_shake_handle) == AME_STATE_RUNNING) {
            shake_offset = AME_GetCurrentValue(s_shake_handle);
        } else if (AME_GetState(s_shake_handle) == AME_STATE_IDLE || AME_GetState(s_shake_handle) == AME_STATE_COMPLETED || AME_GetState(s_shake_handle) == AME_STATE_CANCELLED) {
            AME_DestroyAnimation(s_shake_handle);
            s_shake_handle = AME_INVALID_HANDLE;
        }
    }

    /* 3. Matte Black UI Card */
    int card_w = 460;
    int card_h = 320;
    int card_x = center_x - card_w / 2 + shake_offset;
    int card_y = 205;
    
    draw_box(fb, width, height, card_x - 1, card_y - 1, card_w + 2, card_h + 2, 0xFF222222, 0);
    draw_box(fb, width, height, card_x, card_y, card_w, card_h, 0xFF1A1A1A, 0xFF060606);

    /* 4. Consistent Atom Logo inside Center Card Avatar */
    login_fill_circle(fb, width, height, center_x + shake_offset, card_y + 45, 28, 0xFF0A0A0A);
    login_fill_circle(fb, width, height, center_x + shake_offset, card_y + 45, 27, 0xFF282828);
    login_draw_atom_logo(fb, width, height, center_x + shake_offset, card_y + 45, 24, 8, 4, 20);

    /* Profile Name */
    draw_centered_str(fb, width, height, Identity_GetDefaultUsername(), card_y + 95, 0xFFFFFFFF, 2, 6, shake_offset);
    draw_centered_str(fb, width, height, "ATOMS OS SYSTEM ACCOUNT", card_y + 130, 0xFF888888, 1, 3, shake_offset);

    /* Matte Dark Input Box */
    int input_w = 300;
    int input_h = 42;
    int input_x = center_x - input_w / 2 + shake_offset;
    int input_y = card_y + 170;
    draw_box(fb, width, height, input_x, input_y, input_w, input_h, s_error_state ? 0xFFFF4444 : 0xFF333333, 0xFF020202);

    if (s_password_len == 0) {
        draw_centered_str(fb, width, height, "ENTER PASSWORD...", input_y + 14, 0xFF666666, 1, 4, shake_offset);
    } else {
        char mask_str[128] = "";
        int m_idx = 0;
        for (int i = 0; i < s_password_len && i < 30; i++) {
            mask_str[m_idx++] = '*';
            mask_str[m_idx++] = ' ';
        }
        if (m_idx > 0) mask_str[m_idx - 1] = '\0';
        draw_centered_str(fb, width, height, mask_str, input_y + 14, 0xFFFFFFFF, 1, 4, shake_offset);
    }

    if (s_error_state) {
        draw_centered_str(fb, width, height, "INVALID USERNAME OR PASSWORD.", input_y + 52, 0xFFFF4444, 1, 2, shake_offset);
    }

    /* LOGIN Button */
    int btn_x = center_x - 80 + shake_offset;
    int btn_y = card_y + 250;
    int btn_w = 160;
    int btn_h = 36;
    draw_box(fb, width, height, btn_x, btn_y, btn_w, btn_h, 0xFF555555, 0xFF181818);
    draw_centered_str(fb, width, height, "LOGIN ->", btn_y + 11, 0xFFFFFFFF, 1, 4, shake_offset);

    /* Footer minimalist controls */
    draw_centered_str(fb, width, height, "SHUTDOWN     RESTART     OPTIONS", height - 40, 0xFF555555, 1, 4, 0);
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
