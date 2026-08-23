#include "../include/icon_engine.h"

// Fast Integer Square Root (Pure Freestanding Safe)
static inline uint32_t icon_isqrt(uint32_t n) {
    uint32_t root = 0;
    uint32_t bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit != 0) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

// Sub-pixel Alpha Blending Helper with BWE Clipping
static inline void icon_plot_pixel_blend(const BVFramebuffer* fb, int32_t x, int32_t y,
                                         uint32_t color, const BWE_Rect* clip) {
    if (!clip || (x >= clip->x && x < clip->x + clip->width &&
                  y >= clip->y && y < clip->y + clip->height)) {
        if (x >= 0 && x < (int32_t)fb->width && y >= 0 && y < (int32_t)fb->height) {
            uint32_t alpha = (color >> 24) & 0xFF;
            if (alpha == 0xFF) {
                fb->buffer[y * (fb->pitch / 4) + x] = color;
            } else if (alpha > 0) {
                uint32_t dst = fb->buffer[y * (fb->pitch / 4) + x];
                uint32_t dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;
                uint32_t sr = (color >> 16) & 0xFF, sg = (color >> 8) & 0xFF, sb = color & 0xFF;
                uint32_t r = (sr * alpha + dr * (255 - alpha)) / 255;
                uint32_t g = (sg * alpha + dg * (255 - alpha)) / 255;
                uint32_t b = (sb * alpha + db * (255 - alpha)) / 255;
                fb->buffer[y * (fb->pitch / 4) + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
    }
}

/**
 * @brief Official ATOMS OS V2 Master Start Emblem Procedural Renderer
 * 100% Pure Integer/Fixed-Point Vector Geometry (Zero SSE / Zero FPU)
 */
bool atoms_start_icon_render(const BVFramebuffer* fb, const IconRenderContext* ctx) {
    if (!fb || !fb->buffer || !ctx) return false;

    int32_t w = ctx->width;
    int32_t h = ctx->height;
    int32_t dest_x = ctx->x;
    int32_t dest_y = ctx->y;
    const BWE_Rect* clip = ctx->clip;
    IconState state = ctx->state;

    int32_t center_x = w / 2;
    int32_t center_y = h / 2;
    int32_t max_r = (w < h ? w : h) / 2;
    int32_t max_r_sq = max_r * max_r;

    // Palette Configuration per Interactive State
    uint32_t col_spark = 0xFFFFFFFF; // Pure White Spark
    uint32_t col_core;
    uint32_t col_blade_hi;
    uint32_t col_blade_mid;
    uint32_t col_strut;
    uint32_t col_sat;

    switch (state) {
        case ICON_STATE_HOVER:
            col_core      = 0xFF00E5FF; // High-Voltage Neon Cyan
            col_blade_hi  = 0xFFFFFFFF; // Crisp Pure White
            col_blade_mid = 0xFF38BDF8; // Electric Cyan
            col_strut     = 0xFF7DD3FC; // Radiant Cyan Strut
            col_sat       = 0xFFFFFFFF; // White Node
            break;
        case ICON_STATE_PRESSED:
            col_core      = 0xFF0284C7; // Deep Azure
            col_blade_hi  = 0xFF7DD3FC;
            col_blade_mid = 0xFF0369A1;
            col_strut     = 0xFF0284C7;
            col_sat       = 0xFFBAE6FD;
            break;
        case ICON_STATE_ACTIVE: // Start Menu Open
            col_core      = 0xFF38BDF8; // Vibrant Electric Cyan
            col_blade_hi  = 0xFFFFFFFF;
            col_blade_mid = 0xFF00E5FF;
            col_strut     = 0xFF38BDF8;
            col_sat       = 0xFFFFFFFF;
            break;
        case ICON_STATE_DISABLED:
            col_core      = 0xFF64748B;
            col_blade_hi  = 0xFF94A3B8;
            col_blade_mid = 0xFF475569;
            col_strut     = 0xFF475569;
            col_sat       = 0xFF94A3B8;
            break;
        case ICON_STATE_NORMAL:
        default:
            col_core      = 0xFF38BDF8; // Electric Cyan (#38BDF8)
            col_blade_hi  = 0xFFFFFFFF; // Pure Crisp White
            col_blade_mid = 0xFF00D2FF; // Vibrant Vivid Cyan
            col_strut     = 0xFF0284C7; // Deep Electric Blue
            col_sat       = 0xFFFFFFFF; // Pure White Satellite Node
            break;
    }

    if (ctx->accent_color != 0) {
        col_core = ctx->accent_color;
        col_blade_mid = ctx->accent_color;
    }

    // Geometry Thresholds (Integer Pixel Units Scaled for 28x28)
    // Scale factor: w / 28 in 256 fixed-point
    int32_t scale_256 = (w * 256) / 28;

    // Radius squared thresholds
    int32_t r_core_sq       = (24 * scale_256 * scale_256) / (256 * 256); // radius ~4.9px
    int32_t r_core_spark_sq = (6  * scale_256 * scale_256) / (256 * 256); // radius ~2.4px

    // 3 Satellite Node Centers (Normalized for 28x28):
    // Sat 1: (0, -10) -> Top
    // Sat 2: (+9, +5) -> Bottom-Right
    // Sat 3: (-9, +5) -> Bottom-Left
    int32_t sat1_x = 0;
    int32_t sat1_y = -(10 * scale_256) / 256;

    int32_t sat2_x = (9 * scale_256) / 256;
    int32_t sat2_y = (5 * scale_256) / 256;

    int32_t sat3_x = -(9 * scale_256) / 256;
    int32_t sat3_y = (5 * scale_256) / 256;

    int32_t r_sat_sq = (7 * scale_256 * scale_256) / (256 * 256); // radius ~2.6px

    for (int32_t py = 0; py < h; py++) {
        int32_t dy = py - center_y;

        for (int32_t px = 0; px < w; px++) {
            int32_t dx = px - center_x;
            int32_t r_sq = dx * dx + dy * dy;

            if (r_sq > max_r_sq) continue;

            uint8_t final_a = 0;
            uint8_t final_r = 0;
            uint8_t final_g = 0;
            uint8_t final_b = 0;

            // =========================================================
            // LAYER 1: 3 Radial Energy Struts (Lattice Bridges)
            // =========================================================
            // Strut 1: Vertical (along dx = 0, dy < 0)
            if (dy <= -3 && dy >= sat1_y && dx >= -1 && dx <= 1) {
                final_a = 220;
                final_r = (col_strut >> 16) & 0xFF;
                final_g = (col_strut >> 8) & 0xFF;
                final_b = col_strut & 0xFF;
            }
            // Strut 2: Bottom-Right (approx dx*128 - dy*222 ≈ 0)
            int32_t p2_proj = (dx * 222 + dy * 128) / 256;
            int32_t p2_perp = (dy * 222 - dx * 128) / 256;
            if (p2_perp < 0) p2_perp = -p2_perp;
            if (p2_proj >= 3 && p2_proj <= 10 && p2_perp <= 1) {
                final_a = 220;
                final_r = (col_strut >> 16) & 0xFF;
                final_g = (col_strut >> 8) & 0xFF;
                final_b = col_strut & 0xFF;
            }
            // Strut 3: Bottom-Left (approx -dx*128 - dy*222 ≈ 0)
            int32_t p3_proj = (-dx * 222 + dy * 128) / 256;
            int32_t p3_perp = (dy * 222 + dx * 128) / 256;
            if (p3_perp < 0) p3_perp = -p3_perp;
            if (p3_proj >= 3 && p3_proj <= 10 && p3_perp <= 1) {
                final_a = 220;
                final_r = (col_strut >> 16) & 0xFF;
                final_g = (col_strut >> 8) & 0xFF;
                final_b = col_strut & 0xFF;
            }

            // =========================================================
            // LAYER 2: 3 Dynamic Sweeping Orbital Blades
            // =========================================================
            // Blade 1 (Top Sweep): centered around r = 9.5px, dy in [-12, 1]
            if (r_sq >= 49 && r_sq <= 135 && dy <= 1) {
                int32_t diff = r_sq - 90;
                if (diff < 0) diff = -diff;
                if (diff <= 36) {
                    if (r_sq >= 86) {
                        // Crisp White Outer Leading Edge
                        final_a = 255;
                        final_r = (col_blade_hi >> 16) & 0xFF;
                        final_g = (col_blade_hi >> 8) & 0xFF;
                        final_b = col_blade_hi & 0xFF;
                    } else {
                        // Vivid Cyan Body
                        final_a = 255;
                        final_r = (col_blade_mid >> 16) & 0xFF;
                        final_g = (col_blade_mid >> 8) & 0xFF;
                        final_b = col_blade_mid & 0xFF;
                    }
                }
            }

            // Blade 2 (Bottom-Right Sweep):
            if (r_sq >= 49 && r_sq <= 135 && dx >= -1 && (dy * 148 - dx * 85) <= 120) {
                int32_t diff = r_sq - 90;
                if (diff < 0) diff = -diff;
                if (diff <= 36) {
                    if (r_sq >= 86) {
                        final_a = 255;
                        final_r = (col_blade_hi >> 16) & 0xFF;
                        final_g = (col_blade_hi >> 8) & 0xFF;
                        final_b = col_blade_hi & 0xFF;
                    } else {
                        final_a = 255;
                        final_r = (col_blade_mid >> 16) & 0xFF;
                        final_g = (col_blade_mid >> 8) & 0xFF;
                        final_b = col_blade_mid & 0xFF;
                    }
                }
            }

            // Blade 3 (Bottom-Left Sweep):
            if (r_sq >= 49 && r_sq <= 135 && dx <= 1 && (dy * 148 + dx * 85) <= 120) {
                int32_t diff = r_sq - 90;
                if (diff < 0) diff = -diff;
                if (diff <= 36) {
                    if (r_sq >= 86) {
                        final_a = 255;
                        final_r = (col_blade_hi >> 16) & 0xFF;
                        final_g = (col_blade_hi >> 8) & 0xFF;
                        final_b = col_blade_hi & 0xFF;
                    } else {
                        final_a = 255;
                        final_r = (col_blade_mid >> 16) & 0xFF;
                        final_g = (col_blade_mid >> 8) & 0xFF;
                        final_b = col_blade_mid & 0xFF;
                    }
                }
            }

            // =========================================================
            // LAYER 3: 3 Valence Quantum Satellite Nodes (Crystals)
            // =========================================================
            int32_t d1_sq = (dx - sat1_x) * (dx - sat1_x) + (dy - sat1_y) * (dy - sat1_y);
            int32_t d2_sq = (dx - sat2_x) * (dx - sat2_x) + (dy - sat2_y) * (dy - sat2_y);
            int32_t d3_sq = (dx - sat3_x) * (dx - sat3_x) + (dy - sat3_y) * (dy - sat3_y);

            if (d1_sq <= r_sat_sq || d2_sq <= r_sat_sq || d3_sq <= r_sat_sq) {
                int32_t min_d = d1_sq;
                if (d2_sq < min_d) min_d = d2_sq;
                if (d3_sq < min_d) min_d = d3_sq;

                final_a = 255;
                if (min_d <= 2) {
                    // Pure White Spark
                    final_r = 255; final_g = 255; final_b = 255;
                } else {
                    // Cyan Corona
                    final_r = (col_core >> 16) & 0xFF;
                    final_g = (col_core >> 8) & 0xFF;
                    final_b = col_core & 0xFF;
                }
            }

            // =========================================================
            // LAYER 4: Central Quantum Core (Nucleus)
            // =========================================================
            if (r_sq <= r_core_sq) {
                final_a = 255;
                if (r_sq <= r_core_spark_sq) {
                    // 100% Solid Pure Quantum Spark
                    final_r = 255; final_g = 255; final_b = 255;
                } else {
                    // Electric Cyan Mantle with top-left highlight
                    int32_t dot = -dx - dy;
                    int32_t light = dot > 0 ? (dot * 18) : 0;
                    int32_t cr = ((col_core >> 16) & 0xFF) + light;
                    int32_t cg = ((col_core >> 8) & 0xFF) + light;
                    int32_t cb = (col_core & 0xFF) + light;
                    final_r = (uint8_t)(cr > 255 ? 255 : cr);
                    final_g = (uint8_t)(cg > 255 ? 255 : cg);
                    final_b = (uint8_t)(cb > 255 ? 255 : cb);
                }
            }

            // =========================================================
            // LAYER 5: Outer Subtle Geometric Rim
            // =========================================================
            if (final_a == 0 && r_sq >= (max_r_sq * 85) / 100 && r_sq <= (max_r_sq * 96) / 100) {
                final_a = (state == ICON_STATE_HOVER) ? 140 : 50;
                final_r = (col_core >> 16) & 0xFF;
                final_g = (col_core >> 8) & 0xFF;
                final_b = col_core & 0xFF;
            }

            if (final_a > 0) {
                uint32_t out_color = ((uint32_t)final_a << 24) |
                                     ((uint32_t)final_r << 16) |
                                     ((uint32_t)final_g << 8)  |
                                     (uint32_t)final_b;
                icon_plot_pixel_blend(fb, dest_x + px, dest_y + py, out_color, clip);
            }
        }
    }

    return true;
}
