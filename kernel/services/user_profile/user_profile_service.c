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
    if (!s_user_icon_decoded) {
        s_user_icon_surf = png_decode(g_boot_ico_user_png, sizeof(g_boot_ico_user_png));
        s_user_icon_decoded = true;
    }
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
    if (!s_user_icon_decoded) user_profile_service_init();

    uint32_t stride_pixels = stride / 4;
    if (stride_pixels == 0) stride_pixels = fb_w;

    int r2 = radius * radius;
    int inner_r2 = (radius - 3) * (radius - 3);

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
                uint32_t fg;

                if (dist2 > inner_r2) {
                    /* Outer ring border: Crisp White */
                    fg = 0x00FFFFFF;
                    fb[offset] = blend_alpha(bg, fg, alpha);
                } else {
                    /* Interior fill: Slate glass or user icon pixel */
                    if (s_user_icon_surf && s_user_icon_surf->framebuffer) {
                        int src_w = s_user_icon_surf->width;
                        int src_h = s_user_icon_surf->height;
                        int u = ((dx + radius) * src_w) / (radius * 2);
                        int v = ((dy + radius) * src_h) / (radius * 2);
                        if (u >= 0 && u < src_w && v >= 0 && v < src_h) {
                            fg = s_user_icon_surf->framebuffer[v * src_w + u];
                            fb[offset] = blend_alpha(bg, fg, alpha);
                        }
                    } else {
                        fg = 0x001E293B;
                        fb[offset] = blend_alpha(bg, fg, alpha);
                    }
                }
            }
        }
    }
}
