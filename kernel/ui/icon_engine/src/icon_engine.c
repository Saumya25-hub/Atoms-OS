#include "../include/icon_engine.h"
#include "../include/atoms_icon_data.h"
#include "kernel/core/lib/include/string.h"

/* =========================================================================
 * ATOMS OS — AUTHORITATIVE HIGH-DEFINITION PNG ICON ENGINE
 * Zero-Allocation, Bilinear Downscaler, Sub-Pixel Alpha Compositor
 * ========================================================================= */

// External references for custom vector renderers
extern bool atoms_start_icon_render(const BVFramebuffer* fb, const IconRenderContext* ctx);

// Internal Registry Table
static IconRendererFn s_icon_registry[ICON_ID_MAX] = {0};
static const uint32_t* s_icon_bitmaps[ICON_ID_MAX]  = {0};
static bool           s_engine_initialized = false;

static inline uint32_t clamp_u8(uint32_t val) {
    return val > 255 ? 255 : val;
}

/**
 * @brief High-Performance Integer Bilinear Scaler & Alpha Compositor
 * Zero heap allocations, full BWE clipping bounds compliance.
 */
static bool icon_render_bitmap_scaled(const BVFramebuffer* fb, const uint32_t* src_bitmap,
                                      uint32_t src_w, uint32_t src_h,
                                      const IconRenderContext* ctx) {
    if (!fb || !fb->buffer || !src_bitmap || !ctx) return false;

    int32_t dst_x = ctx->x;
    int32_t dst_y = ctx->y;
    int32_t dst_w = ctx->width;
    int32_t dst_h = ctx->height;
    if (dst_w <= 0 || dst_h <= 0) return false;

    const BWE_Rect* clip = ctx->clip;
    int32_t clip_x1 = 0, clip_y1 = 0;
    int32_t clip_x2 = (int32_t)fb->width, clip_y2 = (int32_t)fb->height;

    if (clip) {
        if (clip->x > clip_x1) clip_x1 = clip->x;
        if (clip->y > clip_y1) clip_y1 = clip->y;
        if (clip->x + clip->width < clip_x2) clip_x2 = clip->x + clip->width;
        if (clip->y + clip->height < clip_y2) clip_y2 = clip->y + clip->height;
    }

    int32_t render_x1 = dst_x;
    int32_t render_y1 = dst_y;
    int32_t render_x2 = dst_x + dst_w;
    int32_t render_y2 = dst_y + dst_h;

    if (render_x1 < clip_x1) render_x1 = clip_x1;
    if (render_y1 < clip_y1) render_y1 = clip_y1;
    if (render_x2 > clip_x2) render_x2 = clip_x2;
    if (render_y2 > clip_y2) render_y2 = clip_y2;

    if (render_x1 >= render_x2 || render_y1 >= render_y2) {
        return true; // Clipped out
    }

    IconState state = ctx->state;
    uint32_t pitch_pixels = fb->pitch / 4;

    for (int32_t dy = render_y1; dy < render_y2; dy++) {
        int32_t out_y = dy - dst_y;
        int32_t fy = (out_y * (int32_t)src_h * 256) / dst_h;
        int32_t y0 = fy >> 8;
        int32_t y_frac = fy & 0xFF;
        int32_t y1 = (y0 + 1 < (int32_t)src_h) ? y0 + 1 : y0;

        uint32_t row_offset = dy * pitch_pixels;

        for (int32_t dx = render_x1; dx < render_x2; dx++) {
            int32_t out_x = dx - dst_x;
            int32_t fx = (out_x * (int32_t)src_w * 256) / dst_w;
            int32_t x0 = fx >> 8;
            int32_t x_frac = fx & 0xFF;
            int32_t x1 = (x0 + 1 < (int32_t)src_w) ? x0 + 1 : x0;

            uint32_t c00 = src_bitmap[y0 * src_w + x0];
            uint32_t c10 = src_bitmap[y0 * src_w + x1];
            uint32_t c01 = src_bitmap[y1 * src_w + x0];
            uint32_t c11 = src_bitmap[y1 * src_w + x1];

            uint32_t a00 = (c00 >> 24) & 0xFF, r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
            uint32_t a10 = (c10 >> 24) & 0xFF, r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
            uint32_t a01 = (c01 >> 24) & 0xFF, r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
            uint32_t a11 = (c11 >> 24) & 0xFF, r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

            uint32_t top_a = ((a00 * (256 - x_frac)) + (a10 * x_frac)) >> 8;
            uint32_t top_r = ((r00 * (256 - x_frac)) + (r10 * x_frac)) >> 8;
            uint32_t top_g = ((g00 * (256 - x_frac)) + (g10 * x_frac)) >> 8;
            uint32_t top_b = ((b00 * (256 - x_frac)) + (b10 * x_frac)) >> 8;

            uint32_t bot_a = ((a01 * (256 - x_frac)) + (a11 * x_frac)) >> 8;
            uint32_t bot_r = ((r01 * (256 - x_frac)) + (r11 * x_frac)) >> 8;
            uint32_t bot_g = ((g01 * (256 - x_frac)) + (g11 * x_frac)) >> 8;
            uint32_t bot_b = ((b01 * (256 - x_frac)) + (b11 * x_frac)) >> 8;

            uint32_t a = ((top_a * (256 - y_frac)) + (bot_a * y_frac)) >> 8;
            if (a < 4) continue; // Transparent pixel skip

            uint32_t r = ((top_r * (256 - y_frac)) + (bot_r * y_frac)) >> 8;
            uint32_t g = ((top_g * (256 - y_frac)) + (bot_g * y_frac)) >> 8;
            uint32_t b = ((top_b * (256 - y_frac)) + (bot_b * y_frac)) >> 8;

            // Interactive state modulation
            if (state == ICON_STATE_HOVER) {
                r = clamp_u8(r + 28);
                g = clamp_u8(g + 28);
                b = clamp_u8(b + 28);
            } else if (state == ICON_STATE_PRESSED) {
                r = (r * 200) / 255;
                g = (g * 200) / 255;
                b = (b * 200) / 255;
            } else if (state == ICON_STATE_ACTIVE) {
                r = clamp_u8((r * 220 + 56 * 35) / 255);
                g = clamp_u8((g * 220 + 189 * 35) / 255);
                b = clamp_u8((b * 220 + 248 * 35) / 255);
            } else if (state == ICON_STATE_DISABLED) {
                uint32_t lum = (r * 77 + g * 150 + b * 29) >> 8;
                r = g = b = lum;
                a = a / 2;
            }

            uint32_t buf_idx = row_offset + dx;
            if (a >= 250) {
                fb->buffer[buf_idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else {
                uint32_t dst = fb->buffer[buf_idx];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t out_r = (r * a + dr * (255 - a)) / 255;
                uint32_t out_g = (g * a + dg * (255 - a)) / 255;
                uint32_t out_b = (b * a + db * (255 - a)) / 255;
                fb->buffer[buf_idx] = 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
            }
        }
    }

    return true;
}

void IconEngine_Initialize(void) {
    if (s_engine_initialized) return;

    for (int i = 0; i < ICON_ID_MAX; i++) {
        s_icon_registry[i] = NULL;
        s_icon_bitmaps[i]  = NULL;
    }

    // Register Official ATOMS Start Emblem
    IconEngine_Register(ICON_ID_ATOMS_START, atoms_start_icon_render);

    // Register Canonical Master 48x48 Bitmaps for All Applications & Systems
    s_icon_bitmaps[ICON_ID_ATOMS_START]    = g_atoms_ico_atoms_start_48;
    s_icon_bitmaps[ICON_ID_COMPUTER]       = g_atoms_ico_computer_48;
    s_icon_bitmaps[ICON_ID_EXPLORER]       = g_atoms_ico_explorer_48;
    s_icon_bitmaps[ICON_ID_TERMINAL]       = g_atoms_ico_terminal_48;
    s_icon_bitmaps[ICON_ID_NOTES]          = g_atoms_ico_notes_48;
    s_icon_bitmaps[ICON_ID_CALCULATOR]     = g_atoms_ico_calculator_48;
    s_icon_bitmaps[ICON_ID_SETTINGS]       = g_atoms_ico_settings_48;
    s_icon_bitmaps[ICON_ID_MEDIA_PLAYER]   = g_atoms_ico_music_48;
    s_icon_bitmaps[ICON_ID_ATRIX]          = g_atoms_ico_atrix_48;
    s_icon_bitmaps[ICON_ID_TASK_MANAGER]   = g_atoms_ico_tmh_48;
    s_icon_bitmaps[ICON_ID_CONTROL_PANEL]  = g_atoms_ico_settings_48;
    s_icon_bitmaps[ICON_ID_DOOM]           = g_atoms_ico_doom_48;
    s_icon_bitmaps[ICON_ID_GRAPH_3D]       = g_atoms_ico_graph3d_48;
    s_icon_bitmaps[ICON_ID_INPUT_LAB]      = g_atoms_ico_inputlab_48;
    s_icon_bitmaps[ICON_ID_FOLDER]         = g_atoms_ico_folder_48;
    s_icon_bitmaps[ICON_ID_RECYCLE_BIN]    = g_atoms_ico_doom_48; // Crisp fallback
    s_icon_bitmaps[ICON_ID_USB_DISK]       = g_atoms_ico_folder_48;

    s_icon_bitmaps[ICON_ID_SYS_ETHERNET]   = g_atoms_ico_ethernet_48;
    s_icon_bitmaps[ICON_ID_SYS_LAN]        = g_atoms_ico_lan_48;
    s_icon_bitmaps[ICON_ID_SYS_WIFI]       = g_atoms_ico_wifi_48;
    s_icon_bitmaps[ICON_ID_SYS_VOLUME]     = g_atoms_ico_volume_48;
    s_icon_bitmaps[ICON_ID_SYS_BATTERY]    = g_atoms_ico_battery_48;
    s_icon_bitmaps[ICON_ID_SYS_BELL]       = g_atoms_ico_notification_48;
    s_icon_bitmaps[ICON_ID_SYS_POWER]      = g_atoms_ico_power_48;
    s_icon_bitmaps[ICON_ID_SYS_SEARCH]     = g_atoms_ico_search_48;

    s_engine_initialized = true;
}

bool IconEngine_Register(IconId id, IconRendererFn renderer) {
    if (id <= ICON_ID_NONE || id >= ICON_ID_MAX || !renderer) {
        return false;
    }
    s_icon_registry[id] = renderer;
    return true;
}

bool IconEngine_HasIcon(IconId id) {
    if (id <= ICON_ID_NONE || id >= ICON_ID_MAX) return false;
    return (s_icon_registry[id] != NULL || s_icon_bitmaps[id] != NULL);
}

bool IconEngine_Render(const BVFramebuffer* fb, IconId id, const IconRenderContext* ctx) {
    if (!fb || !fb->buffer || !ctx) return false;
    if (ctx->width <= 0 || ctx->height <= 0) return false;
    if (id <= ICON_ID_NONE || id >= ICON_ID_MAX) return false;

    if (!s_engine_initialized) {
        IconEngine_Initialize();
    }

    // Resolve BWE Clipping fallback
    BWE_Rect default_clip;
    IconRenderContext local_ctx = *ctx;
    if (!local_ctx.clip) {
        extern bool BWE_GetClip(BWE_Rect* out_rect);
        if (!BWE_GetClip(&default_clip)) {
            default_clip.x = 0;
            default_clip.y = 0;
            default_clip.width = (int32_t)fb->width;
            default_clip.height = (int32_t)fb->height;
        }
        local_ctx.clip = &default_clip;
    }

    // 1. Procedural vector renderer if registered (e.g. ATOMS Start)
    IconRendererFn renderer = s_icon_registry[id];
    if (renderer) {
        return renderer(fb, &local_ctx);
    }

    // 2. Authoritative HD Master Bitmap scaling renderer
    const uint32_t* bitmap = s_icon_bitmaps[id];
    if (bitmap) {
        return icon_render_bitmap_scaled(fb, bitmap, ATOMS_ICON_MASTER_SIZE, ATOMS_ICON_MASTER_SIZE, &local_ctx);
    }

    return false;
}
