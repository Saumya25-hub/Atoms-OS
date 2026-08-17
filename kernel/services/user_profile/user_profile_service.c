#include "kernel/services/user_profile/user_profile_service.h"
#include "kernel/services/wallpaper/boot_assets.h"
#include "kernel/media/bopawn/formats/image_format.h"
#include "kernel/gui/surface/surface.h"

/*
 * 👤 ATOMS OS User Profile Service Implementation
 * Single user "Saumya" placeholder avatar rendering via user.png asset.
 */

static struct BOSSurface* s_user_icon_surf = NULL;
static bool s_user_icon_decoded = false;

void user_profile_service_init(void) {
    s_user_icon_decoded = true;
}

const char* user_profile_service_get_name(void) {
    return "Saumya";
}

static uint32_t blend_alpha(uint32_t bg_color, uint32_t fg_color, uint8_t alpha) {
    if (alpha == 0) return bg_color;
    uint8_t fg_a = (fg_color >> 24) & 0xFF;
    if (fg_a == 0) fg_a = 255;
    uint32_t eff_alpha = ((uint32_t)fg_a * (uint32_t)alpha) / 255;
    if (eff_alpha == 0) return bg_color;

    uint32_t r_bg = (bg_color >> 16) & 0xFF;
    uint32_t g_bg = (bg_color >> 8) & 0xFF;
    uint32_t b_bg = bg_color & 0xFF;

    uint32_t r_fg = (fg_color >> 16) & 0xFF;
    uint32_t g_fg = (fg_color >> 8) & 0xFF;
    uint32_t b_fg = fg_color & 0xFF;

    uint32_t inv = 255 - eff_alpha;
    uint32_t r = (r_bg * inv + r_fg * eff_alpha) / 255;
    uint32_t g = (g_bg * inv + g_fg * eff_alpha) / 255;
    uint32_t b = (b_bg * inv + b_fg * eff_alpha) / 255;

    return (r << 16) | (g << 8) | b;
}

void user_profile_service_render_avatar(uint32_t* fb, uint32_t fb_w, uint32_t fb_h, uint32_t stride, int cx, int cy, int radius, uint8_t alpha) {
    if (!fb || alpha == 0 || radius <= 0) return;

    uint32_t stride_pixels = (stride >= fb_w * 4) ? (stride / 4) : (stride > 0 ? stride : fb_w);
    if (stride_pixels < fb_w) stride_pixels = fb_w;

    int r2 = radius * radius;
    int inner_r2 = (radius - 2) * (radius - 2);

    /* 1. Render Glass Circle & Glowing White Border */
    for (int dy = -radius; dy <= radius; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= (int)fb_h) continue;
        int dy2 = dy * dy;

        for (int dx = -radius; dx <= radius; dx++) {
            int px = cx + dx;
            if (px < 0 || px >= (int)fb_w) continue;

            int dist2 = dx * dx + dy2;
            if (dist2 <= r2) {
                uint32_t offset = py * stride_pixels + px;
                uint32_t bg = fb[offset];

                if (dist2 > inner_r2) {
                    /* Outer ring border: Crisp White (90% alpha) */
                    uint8_t ring_a = (uint8_t)(((uint32_t)alpha * 230) / 255);
                    fb[offset] = blend_alpha(bg, 0x00FFFFFF, ring_a);
                } else {
                    /* Interior fill: Slate Glass Fill (#1e2d41 at 70% alpha) */
                    uint8_t fill_a = (uint8_t)(((uint32_t)alpha * 180) / 255);
                    fb[offset] = blend_alpha(bg, 0x001E2D41, fill_a);
                }
            }
        }
    }

    /* 2. Render 1:1 Pure Ice-White User Silhouette (54x54) */
    extern const uint8_t g_user_avatar_atlas[];
    int av_size = 54;
    int start_x = cx - av_size / 2;
    int start_y = cy - av_size / 2;

    for (int y = 0; y < av_size; y++) {
        int py = start_y + y;
        if (py < 0 || py >= (int)fb_h) continue;
        uint32_t dst_row = py * stride_pixels;

        for (int x = 0; x < av_size; x++) {
            int px = start_x + x;
            if (px < 0 || px >= (int)fb_w) continue;

            uint8_t icon_a = g_user_avatar_atlas[y * av_size + x];
            if (icon_a > 0) {
                uint8_t eff_a = (uint8_t)(((uint32_t)icon_a * (uint32_t)alpha * 245) / (255 * 255));
                fb[dst_row + px] = blend_alpha(fb[dst_row + px], 0x00FFFFFF, eff_a);
            }
        }
    }
}

