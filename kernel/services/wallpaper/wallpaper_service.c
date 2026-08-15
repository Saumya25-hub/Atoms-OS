#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/media/bopawn/bopawn.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/rtc/rtc.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/services/wallpaper/boot_assets.h"
#include "kernel/performance/include/profiler.h"

/*
 * 🖼️ ATOMS OS Wallpaper Service V3.0
 * Zero-Heap Direct Stream Decoder & 10-Wallpaper 1-Minute Non-Repeating Slideshow Engine.
 */

static uint32_t s_wallpaper_canvas_active[1920 * 1080] __attribute__((aligned(16)));
static uint32_t s_wallpaper_canvas_source[1920 * 1080] __attribute__((aligned(16)));
static uint32_t s_wallpaper_canvas_target[1920 * 1080] __attribute__((aligned(16)));

static uint32_t s_selected_wallpaper_id = 0;
static bool     s_wallpaper_initialized = false;

static uint64_t s_wallpaper_timer_ms = 0;
static uint64_t s_transition_elapsed_ms = 0;
static bool     s_is_transitioning = false;

/*
 * Fast Zero-Heap In-Place QOI Stream Decoder
 * Decompresses directly into target canvas with 0 dynamic memory allocations.
 */
static bool decode_qoi_to_canvas(const uint8_t* data, uint32_t size, uint32_t* canvas) {
    if (!data || size < 14 || !canvas) return false;
    if (data[0] != 'q' || data[1] != 'o' || data[2] != 'i' || data[3] != 'f') return false;

    uint32_t w = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    uint32_t h = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

    if (w == 0 || h == 0) return false;

    uint32_t index[64] = {0};
    uint8_t r = 0, g = 0, b = 0, a = 255;
    uint32_t pixel = 0xFF000000;
    uint32_t p = 14;
    uint32_t total_src_pixels = w * h;
    uint32_t px_pos = 0;

    while (px_pos < total_src_pixels && p < size) {
        uint8_t b1 = data[p++];
        int run = 1;

        if (b1 == 0xFE) { /* QOI_OP_RGB */
            r = data[p++];
            g = data[p++];
            b = data[p++];
            pixel = (a << 24) | (r << 16) | (g << 8) | b;
            index[(r * 3 + g * 5 + b * 7 + a * 11) % 64] = pixel;
        } else if (b1 == 0xFF) { /* QOI_OP_RGBA */
            r = data[p++];
            g = data[p++];
            b = data[p++];
            a = data[p++];
            pixel = (a << 24) | (r << 16) | (g << 8) | b;
            index[(r * 3 + g * 5 + b * 7 + a * 11) % 64] = pixel;
        } else if ((b1 & 0xC0) == 0x00) { /* QOI_OP_INDEX */
            uint32_t idx = b1 & 0x3F;
            pixel = index[idx];
            r = (pixel >> 16) & 0xFF;
            g = (pixel >> 8) & 0xFF;
            b = pixel & 0xFF;
            a = (pixel >> 24) & 0xFF;
        } else if ((b1 & 0xC0) == 0x40) { /* QOI_OP_DIFF */
            r = (uint8_t)(r + ((b1 >> 4) & 0x03) - 2);
            g = (uint8_t)(g + ((b1 >> 2) & 0x03) - 2);
            b = (uint8_t)(b + ( b1       & 0x03) - 2);
            pixel = (a << 24) | (r << 16) | (g << 8) | b;
            index[(r * 3 + g * 5 + b * 7 + a * 11) % 64] = pixel;
        } else if ((b1 & 0xC0) == 0x80) { /* QOI_OP_LUMA */
            uint8_t b2 = data[p++];
            int vg = (b1 & 0x3F) - 32;
            r = (uint8_t)(r + vg - 8 + ((b2 >> 4) & 0x0F));
            g = (uint8_t)(g + vg);
            b = (uint8_t)(b + vg - 8 + ( b2       & 0x0F));
            pixel = (a << 24) | (r << 16) | (g << 8) | b;
            index[(r * 3 + g * 5 + b * 7 + a * 11) % 64] = pixel;
        } else if ((b1 & 0xC0) == 0xC0) { /* QOI_OP_RUN */
            run = (b1 & 0x3F) + 1;
        }

        if (w == 240 && h == 135) {
            /* Direct 8x integer scaling to 1920x1080 canvas */
            for (int i = 0; i < run && px_pos < total_src_pixels; i++) {
                uint32_t sx = px_pos % 240;
                uint32_t sy = px_pos / 240;
                uint32_t dx = sx * 8;
                uint32_t dy = sy * 8;

                for (uint32_t yoff = 0; yoff < 8; yoff++) {
                    uint32_t row = (dy + yoff) * 1920 + dx;
                    for (uint32_t xoff = 0; xoff < 8; xoff++) {
                        canvas[row + xoff] = pixel;
                    }
                }
                px_pos++;
            }
        } else if (w == 480 && h == 270) {
            /* Direct 4x integer scaling to 1920x1080 canvas */
            for (int i = 0; i < run && px_pos < total_src_pixels; i++) {
                uint32_t sx = px_pos % 480;
                uint32_t sy = px_pos / 480;
                uint32_t dx = sx * 4;
                uint32_t dy = sy * 4;

                for (uint32_t yoff = 0; yoff < 4; yoff++) {
                    uint32_t row = (dy + yoff) * 1920 + dx;
                    canvas[row] = pixel;
                    canvas[row + 1] = pixel;
                    canvas[row + 2] = pixel;
                    canvas[row + 3] = pixel;
                }
                px_pos++;
            }
        } else if (w == 960 && h == 540) {
            /* Direct 2x integer scaling to 1920x1080 canvas */
            for (int i = 0; i < run && px_pos < total_src_pixels; i++) {
                uint32_t sx = px_pos % 960;
                uint32_t sy = px_pos / 960;
                uint32_t dx = sx * 2;
                uint32_t dy = sy * 2;
                uint32_t row0 = dy * 1920 + dx;
                uint32_t row1 = (dy + 1) * 1920 + dx;

                canvas[row0] = pixel;
                canvas[row0 + 1] = pixel;
                canvas[row1] = pixel;
                canvas[row1 + 1] = pixel;

                px_pos++;
            }
        } else if (w == 1920 && h == 1080) {
            for (int i = 0; i < run && px_pos < total_src_pixels; i++) {
                canvas[px_pos++] = pixel;
            }
        } else {
            px_pos += run;
        }
    }

    return true;
}

static void build_reference_landscape_canvas(uint32_t* canvas) {
    for (uint32_t i = 0; i < 1920 * 1080; i++) {
        canvas[i] = 0xFF0B0F19; /* ATOMS OS Dark Premium Canvas */
    }
}

void wallpaper_service_init(void) {
    if (s_wallpaper_initialized) return;

    display_print("[WALLPAPER SERVICE DIAG] Initializing Wallpaper Service V3.0 (10 Wallpapers)...\n");

    /* Select first random wallpaper on boot */
    uint64_t ticks = timer_get_ticks();
    uint64_t tsc = 0;
    __asm__ volatile("rdtsc" : "=A"(tsc));
    s_selected_wallpaper_id = (uint32_t)((ticks ^ tsc) % BOOT_WALLPAPERS_COUNT);

    if (g_boot_wallpapers_qoi_sizes[s_selected_wallpaper_id] > 0) {
        if (decode_qoi_to_canvas(g_boot_wallpapers_qoi[s_selected_wallpaper_id],
                                 g_boot_wallpapers_qoi_sizes[s_selected_wallpaper_id],
                                 s_wallpaper_canvas_active)) {
            display_print("[WALLPAPER SERVICE DIAG] SUCCESS: Decoded initial wallpaper (0 bytes heap used)!\n");
            s_wallpaper_initialized = true;
            s_wallpaper_timer_ms = 0;
            s_is_transitioning = false;
            return;
        }
    }

    build_reference_landscape_canvas(s_wallpaper_canvas_active);
    s_wallpaper_initialized = true;
    s_wallpaper_timer_ms = 0;
    s_is_transitioning = false;
}

static inline uint64_t get_hw_tsc(void) {
    uint32_t lo = 0, hi = 0;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t s_last_update_tsc = 0;

void wallpaper_service_select_random(void) {
    if (!s_wallpaper_initialized) {
        wallpaper_service_init();
        return;
    }

    /* Choose a non-repeating random next wallpaper index */
    uint32_t next_id = s_selected_wallpaper_id;
    uint64_t seed = get_hw_tsc();

    for (int retry = 0; retry < 100; retry++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        next_id = (uint32_t)((seed >> 32) % BOOT_WALLPAPERS_COUNT);
        if (next_id != s_selected_wallpaper_id) break;
    }
    if (next_id == s_selected_wallpaper_id) {
        next_id = (s_selected_wallpaper_id + 1) % BOOT_WALLPAPERS_COUNT;
    }

    extern void com1_puts(const char* s);
    com1_puts("[WALLPAPER SERVICE] 1-Min Auto-Rotation Triggered! New Index: ");
    char num[8];
    num[0] = '0' + (next_id / 10);
    num[1] = '0' + (next_id % 10);
    num[2] = '\r'; num[3] = '\n'; num[4] = '\0';
    com1_puts(num);

    /* Decode target wallpaper */
    if (g_boot_wallpapers_qoi_sizes[next_id] > 0) {
        decode_qoi_to_canvas(g_boot_wallpapers_qoi[next_id],
                             g_boot_wallpapers_qoi_sizes[next_id],
                             s_wallpaper_canvas_target);

        /* Snapshot current active canvas to source canvas */
        const uint64_t* src64 = (const uint64_t*)s_wallpaper_canvas_active;
        uint64_t* dst64 = (uint64_t*)s_wallpaper_canvas_source;
        for (uint32_t p = 0; p < (1920 * 1080) >> 1; p++) {
            dst64[p] = src64[p];
        }

        s_selected_wallpaper_id = next_id;
        s_is_transitioning = true;
        s_transition_elapsed_ms = 0;
        s_wallpaper_timer_ms = 0;
    }
}

void wallpaper_service_update(uint64_t delta_ms) {
    (void)delta_ms;
    if (!s_wallpaper_initialized) {
        wallpaper_service_init();
    }

    uint64_t now_tsc = get_hw_tsc();
    if (s_last_update_tsc == 0) {
        s_last_update_tsc = now_tsc;
        return;
    }

    uint64_t elapsed_cycles = now_tsc - s_last_update_tsc;
    s_last_update_tsc = now_tsc;

    /* Intel Haswell i3 @ ~3.0-3.4 GHz: ~3,000,000 cycles per ms */
    uint64_t real_ms = elapsed_cycles / 3000000ULL;
    if (real_ms > 1000) real_ms = 1000; // Clamp any pause spikes

    if (!s_is_transitioning) {
        s_wallpaper_timer_ms += real_ms;
        if (s_wallpaper_timer_ms >= 60000) { /* EXACT 60 Real-World Seconds */
            wallpaper_service_select_random();
        }
    } else {
        s_transition_elapsed_ms += real_ms;
        if (s_transition_elapsed_ms >= 1000) { /* 1.0s Transition Complete */
            s_transition_elapsed_ms = 1000;
            s_is_transitioning = false;
            s_wallpaper_timer_ms = 0;

            /* Final 100% target copy */
            const uint64_t* src64 = (const uint64_t*)s_wallpaper_canvas_target;
            uint64_t* dst64 = (uint64_t*)s_wallpaper_canvas_active;
            for (uint32_t p = 0; p < (1920 * 1080) >> 1; p++) {
                dst64[p] = src64[p];
            }
        } else {
            /* Smooth Non-Linear Cubic Ease (Smoothstep) Cross-Fade */
            float p = (float)s_transition_elapsed_ms / 1000.0f;
            float ease = p * p * (3.0f - 2.0f * p);
            uint32_t alpha = (uint32_t)(ease * 255.0f);
            if (alpha > 255) alpha = 255;
            uint32_t inv_alpha = 255u - alpha;

            /* Fast 64-Bit Pair Blending */
            for (uint32_t i = 0; i < 1920 * 1080; i++) {
                uint32_t c1 = s_wallpaper_canvas_source[i];
                uint32_t c2 = s_wallpaper_canvas_target[i];

                uint32_t r = (((c1 >> 16) & 0xFF) * inv_alpha + ((c2 >> 16) & 0xFF) * alpha) / 255;
                uint32_t g = (((c1 >> 8) & 0xFF) * inv_alpha + ((c2 >> 8) & 0xFF) * alpha) / 255;
                uint32_t b = ((c1 & 0xFF) * inv_alpha + (c2 & 0xFF) * alpha) / 255;
                s_wallpaper_canvas_active[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

uint32_t wallpaper_service_get_selected_id(void) {
    return s_selected_wallpaper_id;
}

const uint32_t* wallpaper_service_get_canvas(void) {
    if (!s_wallpaper_initialized) wallpaper_service_init();
    return s_wallpaper_canvas_active;
}

void wallpaper_service_render(uint32_t* target_fb, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride) {
    BOS_PROFILE_SCOPE("wallpaper_service_render");
    if (!target_fb || fb_width == 0 || fb_height == 0) return;

    if (!s_wallpaper_initialized) wallpaper_service_init();

    /* Strict Surface Invariant: RAM canvas is ALWAYS dense (stride == width) */
    uint32_t stride_pixels = fb_width;

    if (fb_width == 1920 && fb_height == 1080) {
        /* 1:1 Fast 64-Bit QWORD SIMD Pair Transfers (2 Pixels per CPU Store) */
        for (uint32_t y = 0; y < 1080; y++) {
            uint32_t dst_offset = y * stride_pixels;
            uint32_t src_offset = y * 1920;

            uint64_t* dst64 = (uint64_t*)&target_fb[dst_offset];
            const uint64_t* src64 = (const uint64_t*)&s_wallpaper_canvas_active[src_offset];
            for (uint32_t p = 0; p < (1920 >> 1); p++) {
                dst64[p] = src64[p];
            }
        }
    } else {
        /* Universal Aspect-Ratio Proportional Scaler */
        for (uint32_t dy = 0; dy < fb_height; dy++) {
            uint32_t sy = (dy * 1080) / fb_height;
            if (sy >= 1080) sy = 1079;
            uint32_t src_row = sy * 1920;
            uint32_t dst_row = dy * stride_pixels;

            for (uint32_t dx = 0; dx < fb_width; dx++) {
                uint32_t sx = (dx * 1920) / fb_width;
                if (sx >= 1920) sx = 1919;
                target_fb[dst_row + dx] = s_wallpaper_canvas_active[src_row + sx];
            }
        }
    }
}
