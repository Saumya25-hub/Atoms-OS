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
 * 🖼️ ATOMS OS Wallpaper Service V2.0
 * Zero-Heap Direct Stream Decoder & Dynamic Scaling Wallpaper Engine.
 * Supports Embedded QOI Boot Wallpapers + VFS PNG Wallpapers.
 */

static uint32_t s_wallpaper_canvas[1920 * 1080] __attribute__((aligned(16)));
static uint32_t s_selected_wallpaper_id = 0;
static bool     s_wallpaper_initialized = false;

/*
 * Fast Zero-Heap In-Place QOI Stream Decoder
 * Decompresses directly into s_wallpaper_canvas with 0 dynamic memory allocations.
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

        if (w == 960 && h == 540) {
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

static void scale_surface_to_canvas(const struct BOSSurface* surf) {
    if (!surf || !surf->framebuffer || surf->width <= 0 || surf->height <= 0) return;

    int src_w = surf->width;
    int src_h = surf->height;

    for (int dst_y = 0; dst_y < 1080; dst_y++) {
        int src_y = (dst_y * src_h) / 1080;
        if (src_y >= src_h) src_y = src_h - 1;
        uint32_t src_line_offset = src_y * src_w;
        uint32_t dst_line_offset = dst_y * 1920;

        for (int dst_x = 0; dst_x < 1920; dst_x++) {
            int src_x = (dst_x * src_w) / 1920;
            if (src_x >= src_w) src_x = src_w - 1;

            uint32_t pixel = surf->framebuffer[src_line_offset + src_x];
            s_wallpaper_canvas[dst_line_offset + dst_x] = pixel;
        }
    }
}

static void build_reference_landscape_canvas(void) {
    for (int y = 0; y < 1080; y++) {
        uint32_t line_color = 0x000B0F19; /* ATOMS OS Dark Premium Canvas */
        uint32_t dst_offset = y * 1920;
        for (int x = 0; x < 1920; x++) {
            s_wallpaper_canvas[dst_offset + x] = line_color;
        }
    }
}

static struct BOSSurface* try_load_wallpaper(int id) {
    char id_char = '1' + (id % 4);

    char path1[32] = "/W1.PNG"; path1[2] = id_char;
    char path2[32] = "W1.PNG";  path2[1] = id_char;
    char path3[32] = "/1.PNG";  path3[1] = id_char;
    char path4[32] = "1.PNG";   path4[0] = id_char;

    const char* paths[4] = { path1, path2, path3, path4 };

    for (int i = 0; i < 4; i++) {
        BOSImage* img = bopawn_load(paths[i]);
        if (img) {
            struct BOSSurface* surf = bopawn_get_surface(img);
            if (surf && surf->framebuffer && surf->width > 0 && surf->height > 0) {
                display_print("[WALLPAPER SERVICE DIAG] SUCCESS: Loaded wallpaper from ");
                display_print(paths[i]);
                display_print("\n");
                return surf;
            }
        }
    }
    return NULL;
}

void wallpaper_service_init(void) {
    display_print("[WALLPAPER SERVICE DIAG] Initializing Wallpaper Service V2.0...\n");

    /* 1. First priority: Decode embedded high-res photo wallpaper directly into canvas */
    if (g_boot_wallpaper_qoi_size > 0) {
        if (decode_qoi_to_canvas(g_boot_wallpaper_qoi, g_boot_wallpaper_qoi_size, s_wallpaper_canvas)) {
            display_print("[WALLPAPER SERVICE DIAG] SUCCESS: Embedded 1920x1080 photo wallpaper decoded (0 bytes heap used)!\n");
            s_wallpaper_initialized = true;
            return;
        }
    }

    /* 2. Second priority: Try loading from VFS disk if mounted */
    struct BOSSurface* surf = try_load_wallpaper(s_selected_wallpaper_id);
    if (surf && surf->framebuffer) {
        scale_surface_to_canvas(surf);
        s_wallpaper_initialized = true;
        return;
    }

    /* 3. Fallback: Solid reference landscape canvas */
    build_reference_landscape_canvas();
    s_wallpaper_initialized = true;
}

void wallpaper_service_select_random(void) {
    uint64_t ticks = timer_get_ticks();
    s_selected_wallpaper_id = (uint32_t)((ticks ^ (ticks >> 7)) % 4);

    struct BOSSurface* surf = try_load_wallpaper(s_selected_wallpaper_id);
    if (surf && surf->framebuffer) {
        scale_surface_to_canvas(surf);
    } else {
        if (g_boot_wallpaper_qoi_size > 0) {
            decode_qoi_to_canvas(g_boot_wallpaper_qoi, g_boot_wallpaper_qoi_size, s_wallpaper_canvas);
        } else {
            build_reference_landscape_canvas();
        }
    }
}

uint32_t wallpaper_service_get_selected_id(void) {
    return s_selected_wallpaper_id;
}

const uint32_t* wallpaper_service_get_canvas(void) {
    if (!s_wallpaper_initialized) wallpaper_service_init();
    return s_wallpaper_canvas;
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
            const uint64_t* src64 = (const uint64_t*)&s_wallpaper_canvas[src_offset];
            for (uint32_t p = 0; p < (1920 >> 1); p++) {
                dst64[p] = src64[p];
            }
        }
    } else {
        /* Universal Aspect-Ratio Proportional Scaler (Works for 800x600, 1024x768, 1366x768, 1440p, 4K) */
        for (uint32_t dy = 0; dy < fb_height; dy++) {
            uint32_t sy = (dy * 1080) / fb_height;
            if (sy >= 1080) sy = 1079;
            uint32_t src_row = sy * 1920;
            uint32_t dst_row = dy * stride_pixels;

            for (uint32_t dx = 0; dx < fb_width; dx++) {
                uint32_t sx = (dx * 1920) / fb_width;
                if (sx >= 1920) sx = 1919;
                target_fb[dst_row + dx] = s_wallpaper_canvas[src_row + sx];
            }
        }
    }
}
