#include "bos_shell_panel.h"
#include "kernel/core/lib/include/string.h"

static inline void plot_pixel(const BVFramebuffer* fb, int32_t x, int32_t y, uint32_t color, const BWE_Rect* clip) {
    if (x >= clip->x && x < clip->x + clip->width && y >= clip->y && y < clip->y + clip->height) {
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

static inline bool is_outside_rounded_rect(int32_t x, int32_t y, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r) {
    int32_t left_cx = rx + r;
    int32_t right_cx = rx + rw - r - 1;
    int32_t top_cy = ry + r;
    int32_t bottom_cy = ry + rh - r - 1;

    if (x < left_cx && y < top_cy) {
        int32_t dx = x - left_cx;
        int32_t dy = y - top_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x > right_cx && y < top_cy) {
        int32_t dx = x - right_cx;
        int32_t dy = y - top_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x < left_cx && y > bottom_cy) {
        int32_t dx = x - left_cx;
        int32_t dy = y - bottom_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    if (x > right_cx && y > bottom_cy) {
        int32_t dx = x - right_cx;
        int32_t dy = y - bottom_cy;
        return (dx * dx + dy * dy) > (r * r);
    }
    return false;
}

// Single Elegant Layered Drop Shadow & 1px Crisp Border (Zero Square Dark Ears)
void draw_bos_panel(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t bg_color, uint32_t border_color, const BWE_Rect* clip) {
    // 1. Layered Alpha Shadow strictly outside panel rounded boundary
    for (int32_t s = 3; s >= 1; s--) {
        uint32_t alpha = 0x0E * (4 - s);
        for (int32_t y = ry - s; y < ry + rh + s + 3; y++) {
            for (int32_t x = rx - s; x < rx + rw + s; x++) {
                if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) {
                    if (!is_outside_rounded_rect(x - s, y - s - 1, rx, ry, rw, rh, r)) {
                        plot_pixel(fb, x, y, (alpha << 24) | 0x000000, clip);
                    }
                }
            }
        }
    }

    // 2. Main Surface & 1px Crisp Border
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) continue;

            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, r));

            plot_pixel(fb, x, y, is_edge ? border_color : bg_color, clip);
        }
    }
}

// Premium Glassmorphic Capsule Renderer V1.1 (Dark ATOMS Glass Normal / Frosted Glow Hover)
void draw_bos_glass_capsule(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, bool is_hovered, const BWE_Rect* clip) {
    // 1. Soft Ambient Drop Shadow
    for (int32_t s = 2; s >= 1; s--) {
        uint32_t alpha = 0x0C * (3 - s);
        for (int32_t y = ry - s; y < ry + rh + s + 2; y++) {
            for (int32_t x = rx - s; x < rx + rw + s; x++) {
                if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) {
                    if (!is_outside_rounded_rect(x - s, y - s - 1, rx, ry, rw, rh, r)) {
                        plot_pixel(fb, x, y, (alpha << 24) | 0x000000, clip);
                    }
                }
            }
        }
    }

    // Normal State: Dark semi-transparent ATOMS glass matching Start Menu & Calendar palette
    // Hover State: Lighter frosted glass appearance with subtle white border & soft inner glow
    uint32_t glass_bg     = is_hovered ? 0x331E293B : 0xFA0F172A; 
    uint32_t glass_border = is_hovered ? 0x55FFFFFF : 0xFF334155; 
    uint32_t inner_glow   = is_hovered ? 0x20FFFFFF : 0xFA0F172A;

    // 2. Surface Rendering
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) continue;

            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, r) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, r));

            bool is_inner_edge = !is_edge &&
                                 (x == rx + 1 || x == rx + rw - 2 || y == ry + 1 || y == ry + rh - 2 ||
                                  is_outside_rounded_rect(x - 2, y, rx, ry, rw, rh, r) ||
                                  is_outside_rounded_rect(x + 2, y, rx, ry, rw, rh, r) ||
                                  is_outside_rounded_rect(x, y - 2, rx, ry, rw, rh, r) ||
                                  is_outside_rounded_rect(x, y + 2, rx, ry, rw, rh, r));

            if (is_edge) {
                plot_pixel(fb, x, y, glass_border, clip);
            } else if (is_inner_edge) {
                plot_pixel(fb, x, y, inner_glow, clip);
            } else {
                plot_pixel(fb, x, y, glass_bg, clip);
            }
        }
    }
}

void draw_bos_rounded_box(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t r, uint32_t color, const BWE_Rect* clip) {
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (!is_outside_rounded_rect(x, y, rx, ry, rw, rh, r)) {
                plot_pixel(fb, x, y, color, clip);
            }
        }
    }
}

void draw_bos_separator(const BVFramebuffer* fb, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, const BWE_Rect* clip) {
    if (y1 == y2) {
        for (int32_t x = x1; x <= x2; x++) plot_pixel(fb, x, y1, color, clip);
    } else if (x1 == x2) {
        for (int32_t y = y1; y <= y2; y++) plot_pixel(fb, x1, y, color, clip);
    }
}

// Thin Track Slider with Optical Centered Knob
void draw_bos_slider(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, int32_t percent, bool is_hovered, bool is_dragging, uint32_t accent_color, const BWE_Rect* clip) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    int32_t track_h = 4;
    int32_t track_y = ry + (rh - track_h) / 2;

    // Track Background
    draw_bos_rounded_box(fb, rx, track_y, rw, track_h, 2, 0xFF334155, clip);

    // Active Fill Bar
    int32_t fill_w = (rw * percent) / 100;
    if (fill_w > 0) {
        draw_bos_rounded_box(fb, rx, track_y, fill_w, track_h, 2, accent_color, clip);
    }

    // Knob
    int32_t knob_r = is_dragging ? 5 : (is_hovered ? 5 : 4);
    int32_t knob_cx = rx + fill_w;
    if (knob_cx < rx) knob_cx = rx;
    if (knob_cx > rx + rw) knob_cx = rx + rw;
    int32_t knob_cy = ry + rh / 2;

    for (int32_t y = knob_cy - knob_r - 1; y <= knob_cy + knob_r + 1; y++) {
        for (int32_t x = knob_cx - knob_r - 1; x <= knob_cx + knob_r + 1; x++) {
            int32_t dx = x - knob_cx;
            int32_t dy = y - knob_cy;
            int32_t dist_sq = dx * dx + dy * dy;
            if (dist_sq <= knob_r * knob_r) {
                uint32_t col = (dist_sq >= (knob_r - 1) * (knob_r - 1)) ? 0xFFFFFFFF : accent_color;
                plot_pixel(fb, x, y, col, clip);
            }
        }
    }
}

// Quick Action Tile
void draw_bos_tile(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, const char* title, const char* status_text, uint32_t icon_id, bool is_on, bool is_hovered, bool is_disabled, const BWE_Rect* clip) {
    uint32_t bg_col;
    uint32_t border_col;

    if (is_disabled) {
        bg_col = 0x1A1E293B;
        border_col = 0xFF334155;
    } else if (is_on) {
        bg_col = is_hovered ? 0x4D2563EB : 0x332563EB; // Restrained BOTHEME accent
        border_col = 0xFF3B82F6;
    } else {
        bg_col = is_hovered ? 0x2AFFFFFF : 0x1A1E293B;
        border_col = 0xFF334155;
    }

    draw_bos_rounded_box(fb, rx, ry, rw, rh, 10, bg_col, clip);

    // 1px Border
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, 10)) continue;
            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, 10));
            if (is_edge) plot_pixel(fb, x, y, border_col, clip);
        }
    }

    // Icon Badge Container
    int32_t icon_box_x = rx + 10;
    int32_t icon_box_y = ry + (rh - 24) / 2;
    uint32_t icon_bg = is_disabled ? 0xFF334155 : (is_on ? 0xFF2563EB : 0xFF334155);
    draw_bos_rounded_box(fb, icon_box_x, icon_box_y, 24, 24, 12, icon_bg, clip);

    int32_t icx = icon_box_x + 12;
    int32_t icy = icon_box_y + 12;

    if (icon_id == ICON_SYS_WIFI_CONN || icon_id == ICON_SYS_WIFI_WEAK || icon_id == ICON_SYS_WIFI_DISC) {
        draw_vector_wifi(fb, icx, icy, 0xFFFFFFFF, clip);
    } else if (icon_id == ICON_SYS_BAT_NORM || icon_id == ICON_SYS_BAT_CHG || icon_id == ICON_SYS_BAT_LOW) {
        draw_vector_battery(fb, icx, icy, 0xFFFFFFFF, clip);
    } else if (icon_id > 0) {
        BOAsset_DrawAsset(icon_id, icon_box_x + 3, icon_box_y + 3, 18, 18);
    }

    // Text Labels
    uint32_t title_col = is_disabled ? 0xFF64748B : 0xFFF1F5F9;
    uint32_t status_col = is_disabled ? 0xFF475569 : (is_on ? 0xFF93C5FD : 0xFF94A3B8);

    BWE_DrawText(fb, title, rx + 40, ry + 7, title_col, 0);
    if (status_text) {
        BWE_DrawText(fb, status_text, rx + 40, ry + 23, status_col, 0);
    }
}

// Premium Message Card
void draw_bos_notification_card(const BVFramebuffer* fb, int32_t rx, int32_t ry, int32_t rw, int32_t rh, const char* title, const char* message, const char* time_str, uint32_t app_icon_id, bool is_hovered, const BWE_Rect* clip) {
    uint32_t bg_col = is_hovered ? 0x2AFFFFFF : 0x1A1E293B;
    draw_bos_rounded_box(fb, rx, ry, rw, rh, 10, bg_col, clip);

    // 1px Border
    for (int32_t y = ry; y < ry + rh; y++) {
        for (int32_t x = rx; x < rx + rw; x++) {
            if (is_outside_rounded_rect(x, y, rx, ry, rw, rh, 10)) continue;
            bool is_edge = (x == rx || x == rx + rw - 1 || y == ry || y == ry + rh - 1 ||
                            is_outside_rounded_rect(x - 1, y, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x + 1, y, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x, y - 1, rx, ry, rw, rh, 10) ||
                            is_outside_rounded_rect(x, y + 1, rx, ry, rw, rh, 10));
            if (is_edge) plot_pixel(fb, x, y, 0xFF334155, clip);
        }
    }

    // App Icon Badge Container
    int32_t icon_x = rx + 10;
    int32_t icon_y = ry + 10;
    draw_bos_rounded_box(fb, icon_x, icon_y, 22, 22, 5, 0xFF2563EB, clip);
    if (app_icon_id > 0) {
        BOAsset_DrawAsset(app_icon_id, icon_x + 3, icon_y + 3, 16, 16);
    }

    // Title
    BWE_DrawText(fb, title ? title : "Notification", rx + 38, ry + 7, 0xFFF1F5F9, 0);

    // Timestamp right-aligned
    if (time_str) {
        int t_len = strlen(time_str);
        BWE_DrawText(fb, time_str, rx + rw - (t_len * 8) - 10, ry + 7, 0xFF64748B, 0);
    }

    // Message snippet
    if (message) {
        char snippet[48];
        strncpy(snippet, message, 44);
        snippet[44] = '\0';
        BWE_DrawText(fb, snippet, rx + 38, ry + 23, 0xFF94A3B8, 0);
    }

    // Hover Dismiss Button 'x'
    if (is_hovered) {
        BWE_DrawText(fb, "x", rx + rw - 14, ry + rh - 16, 0xFF94A3B8, 0);
    }
}

// Procedural System Vector Icons (Image 2 High-Fidelity Match)

void draw_vector_wifi(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip) {
    // 1. Outer Arc (Radius 7)
    for (int dx = -6; dx <= 6; dx++) {
        int dy = (dx == -6 || dx == 6) ? 0 : ((dx == -5 || dx == 5 || dx == -4 || dx == 4) ? -2 : -4);
        plot_pixel(fb, cx + dx, cy + dy, color, clip);
        plot_pixel(fb, cx + dx, cy + dy + 1, color, clip);
    }
    // 2. Middle Arc (Radius 4)
    for (int dx = -3; dx <= 3; dx++) {
        int dy = (dx == -3 || dx == 3) ? 0 : -2;
        plot_pixel(fb, cx + dx, cy + dy, color, clip);
    }
    // 3. Center Dot Node
    plot_pixel(fb, cx - 1, cy + 3, color, clip);
    plot_pixel(fb, cx,     cy + 3, color, clip);
    plot_pixel(fb, cx + 1, cy + 3, color, clip);
    plot_pixel(fb, cx,     cy + 4, color, clip);
}

void draw_vector_volume(const BVFramebuffer* fb, int32_t cx, int32_t cy, bool is_muted, uint32_t color, const BWE_Rect* clip) {
    // Speaker Rect Body
    for (int y = -2; y <= 2; y++) {
        for (int x = -6; x <= -3; x++) {
            plot_pixel(fb, cx + x, cy + y, color, clip);
        }
    }
    // Speaker Cone Triangle
    plot_pixel(fb, cx - 2, cy - 3, color, clip); plot_pixel(fb, cx - 2, cy + 3, color, clip);
    plot_pixel(fb, cx - 1, cy - 4, color, clip); plot_pixel(fb, cx - 1, cy + 4, color, clip);
    plot_pixel(fb, cx,     cy - 5, color, clip); plot_pixel(fb, cx,     cy + 5, color, clip);
    for (int y = -4; y <= 4; y++) plot_pixel(fb, cx - 2, cy + y, color, clip);
    for (int y = -5; y <= 5; y++) plot_pixel(fb, cx - 1, cy + y, color, clip);
    for (int y = -5; y <= 5; y++) plot_pixel(fb, cx,     cy + y, color, clip);

    if (is_muted) {
        uint32_t red = 0xFFEF4444;
        for (int i = -3; i <= 3; i++) {
            plot_pixel(fb, cx + 4 + i, cy + i, red, clip);
            plot_pixel(fb, cx + 4 + i, cy - i, red, clip);
        }
    } else {
        // Sound Wave 1 (Inner Arc)
        plot_pixel(fb, cx + 3, cy - 2, color, clip);
        plot_pixel(fb, cx + 4, cy - 1, color, clip);
        plot_pixel(fb, cx + 4, cy,     color, clip);
        plot_pixel(fb, cx + 4, cy + 1, color, clip);
        plot_pixel(fb, cx + 3, cy + 2, color, clip);

        // Sound Wave 2 (Outer Arc)
        plot_pixel(fb, cx + 6, cy - 4, color, clip);
        plot_pixel(fb, cx + 7, cy - 3, color, clip);
        plot_pixel(fb, cx + 8, cy - 1, color, clip);
        plot_pixel(fb, cx + 8, cy,     color, clip);
        plot_pixel(fb, cx + 8, cy + 1, color, clip);
        plot_pixel(fb, cx + 7, cy + 3, color, clip);
        plot_pixel(fb, cx + 6, cy + 4, color, clip);
    }
}

void draw_vector_battery(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip) {
    int32_t bx = cx - 7;
    int32_t by = cy - 4;
    int32_t bw = 14;
    int32_t bh = 9;

    // 1px White Frame Outline
    for (int x = bx; x < bx + bw; x++) {
        plot_pixel(fb, x, by, color, clip);
        plot_pixel(fb, x, by + bh - 1, color, clip);
    }
    for (int y = by; y < by + bh; y++) {
        plot_pixel(fb, bx, y, color, clip);
        plot_pixel(fb, bx + bw - 1, y, color, clip);
    }

    // Right Terminal Cap Nub
    for (int y = by + 2; y <= by + 6; y++) {
        plot_pixel(fb, bx + bw, y, color, clip);
    }

    // Vibrant Green Charge Level Fill (85% filled)
    uint32_t green = 0xFF22C55E;
    for (int y = by + 2; y <= by + 6; y++) {
        for (int x = bx + 2; x <= bx + 10; x++) {
            plot_pixel(fb, x, y, green, clip);
        }
    }
}

void draw_vector_bell(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip) {
    // Top Loop
    plot_pixel(fb, cx, cy - 6, color, clip);

    // Bell Dome Silhouette (Solid White Fill)
    for (int y = -5; y <= 2; y++) {
        int w = (y <= -4) ? 1 : ((y <= -2) ? 3 : ((y <= 0) ? 5 : 6));
        for (int x = -w; x <= w; x++) {
            plot_pixel(fb, cx + x, cy + y, color, clip);
        }
    }

    // Bottom Rim Line
    for (int x = -7; x <= 7; x++) {
        plot_pixel(fb, cx + x, cy + 3, color, clip);
    }

    // Clapper Node
    plot_pixel(fb, cx - 1, cy + 4, color, clip);
    plot_pixel(fb, cx,     cy + 4, color, clip);
    plot_pixel(fb, cx + 1, cy + 4, color, clip);
    plot_pixel(fb, cx,     cy + 5, color, clip);
}

void draw_vector_sun(const BVFramebuffer* fb, int32_t cx, int32_t cy, uint32_t color, const BWE_Rect* clip) {
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {
            plot_pixel(fb, x, y, color, clip);
        }
    }
    plot_pixel(fb, cx, cy - 5, color, clip);
    plot_pixel(fb, cx, cy + 5, color, clip);
    plot_pixel(fb, cx - 5, cy, color, clip);
    plot_pixel(fb, cx + 5, cy, color, clip);
    plot_pixel(fb, cx - 3, cy - 3, color, clip);
    plot_pixel(fb, cx + 3, cy - 3, color, clip);
    plot_pixel(fb, cx - 3, cy + 3, color, clip);
    plot_pixel(fb, cx + 3, cy + 3, color, clip);
}
