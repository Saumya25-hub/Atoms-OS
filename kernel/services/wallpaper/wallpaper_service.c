#include "kernel/services/wallpaper/wallpaper_service.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/media/bopawn/bopawn.h"
#include "kernel/gui/surface/surface.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/rtc/rtc.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

/*
 * 🖼️ ATOMS OS Wallpaper Service
 * Decodes real PNG wallpapers from BOOT-WALLAPPERS / VFS via bopawn_load().
 * Random selection on boot, static canvas caching, 0% runtime flicker.
 */

static uint32_t s_wallpaper_canvas[1920 * 1080] __attribute__((aligned(16)));
static uint32_t s_selected_wallpaper_id = 0;
static bool     s_wallpaper_initialized = false;

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

static uint32_t blend_c(uint32_t c1, uint32_t c2, int factor_256) {
    if (factor_256 <= 0) return c1;
    if (factor_256 >= 256) return c2;
    int inv = 256 - factor_256;

    uint32_t r = (((c1 >> 16) & 0xFF) * inv + ((c2 >> 16) & 0xFF) * factor_256) >> 8;
    uint32_t g = (((c1 >> 8) & 0xFF) * inv + ((c2 >> 8) & 0xFF) * factor_256) >> 8;
    uint32_t b = ((c1 & 0xFF) * inv + (c2 & 0xFF) * factor_256) >> 8;

    return (r << 16) | (g << 8) | b;
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

    display_print("[WALLPAPER SERVICE DIAG] WARNING: VFS PNG read returned NULL. Using landscape canvas.\n");
    return NULL;
}

void wallpaper_service_init(void) {
    display_print("[WALLPAPER SERVICE DIAG] Initializing Wallpaper Service...\n");
    display_print("[WALLPAPER SERVICE DIAG] Folder: BOOT-WALLAPPERS (1.png, 2.png, 3.png, 4.png)\n");

    uint64_t ticks = timer_get_ticks();
    RTCDateTime dt;
    if (rtc_read_datetime(&dt)) {
        ticks += dt.second + dt.minute * 60;
    }
    s_selected_wallpaper_id = (uint32_t)(ticks % 4);

    display_print("[WALLPAPER SERVICE DIAG] Random Selected Wallpaper Index: ");
    char num_buf[16];
    num_buf[0] = '1' + (s_selected_wallpaper_id % 4);
    num_buf[1] = '\0';
    display_print(num_buf);
    display_print(".png\n");

    struct BOSSurface* surf = try_load_wallpaper(s_selected_wallpaper_id);

    if (surf && surf->framebuffer) {
        scale_surface_to_canvas(surf);
    } else {
        build_reference_landscape_canvas();
    }

    s_wallpaper_initialized = true;
}

void wallpaper_service_select_random(void) {
    uint64_t ticks = timer_get_ticks();
    s_selected_wallpaper_id = (uint32_t)((ticks ^ (ticks >> 7)) % 4);

    struct BOSSurface* surf = try_load_wallpaper(s_selected_wallpaper_id);

    if (surf && surf->framebuffer) {
        scale_surface_to_canvas(surf);
    } else {
        build_reference_landscape_canvas();
    }
}

uint32_t wallpaper_service_get_selected_id(void) {
    return s_selected_wallpaper_id;
}

const uint32_t* wallpaper_service_get_canvas(void) {
    if (!s_wallpaper_initialized) wallpaper_service_init();
    return s_wallpaper_canvas;
}

#include "kernel/performance/include/profiler.h"

void wallpaper_service_render(uint32_t* target_fb, uint32_t fb_width, uint32_t fb_height, uint32_t fb_stride) {
    BOS_PROFILE_SCOPE("wallpaper_service_render");
    if (!target_fb || fb_width == 0 || fb_height == 0) return;

    if (!s_wallpaper_initialized) wallpaper_service_init();

    uint32_t stride_pixels = fb_stride / 4;
    if (stride_pixels == 0) stride_pixels = fb_width;

    uint32_t copy_w = (fb_width < 1920) ? fb_width : 1920;
    uint32_t copy_h = (fb_height < 1080) ? fb_height : 1080;

    for (uint32_t y = 0; y < copy_h; y++) {
        uint32_t dst_offset = y * stride_pixels;
        uint32_t src_offset = y * 1920;
        for (uint32_t x = 0; x < copy_w; x++) {
            target_fb[dst_offset + x] = s_wallpaper_canvas[src_offset + x];
        }
    }
}
