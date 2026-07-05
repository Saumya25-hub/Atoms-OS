#include "painter.h"

// Check if a point is within the clip rectangle
static inline bool is_clipped(int x, int y, const BVRect* clip) {
    if (!clip) return false;
    return (x < clip->x || y < clip->y || x >= clip->x + clip->width || y >= clip->y + clip->height);
}

void painter_draw_pixel(struct BOSSurface* surface, int x, int y, BOVISUAL_Color color, const BVRect* clip) {
    if (!surface || !surface->framebuffer) return;
    
    if (x < 0 || y < 0 || x >= surface->width || y >= surface->height) return;
    if (is_clipped(x, y, clip)) return;

    surface->framebuffer[y * surface->width + x] = color;
}

void painter_draw_line(struct BOSSurface* surface, int x0, int y0, int x1, int y1, BOVISUAL_Color color, const BVRect* clip) {
    if (!surface) return;
    int dx = x1 - x0 > 0 ? x1 - x0 : x0 - x1;
    int dy = y1 - y0 > 0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        painter_draw_pixel(surface, x0, y0, color, clip);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void painter_draw_rect(struct BOSSurface* surface, const BVRect* rect, BOVISUAL_Color color, const BVRect* clip) {
    if (!surface || !rect) return;
    int x2 = rect->x + rect->width - 1;
    int y2 = rect->y + rect->height - 1;
    
    painter_draw_line(surface, rect->x, rect->y, x2, rect->y, color, clip);
    painter_draw_line(surface, rect->x, y2, x2, y2, color, clip);
    painter_draw_line(surface, rect->x, rect->y, rect->x, y2, color, clip);
    painter_draw_line(surface, x2, rect->y, x2, y2, color, clip);
}

void painter_fill_rect(struct BOSSurface* surface, const BVRect* rect, BOVISUAL_Color color, const BVRect* clip) {
    if (!surface || !surface->framebuffer || !rect) return;
    
    BVRect draw_rect = *rect;
    
    // Clip against surface bounds
    if (draw_rect.x < 0) { draw_rect.width += draw_rect.x; draw_rect.x = 0; }
    if (draw_rect.y < 0) { draw_rect.height += draw_rect.y; draw_rect.y = 0; }
    if (draw_rect.x + draw_rect.width > surface->width) draw_rect.width = surface->width - draw_rect.x;
    if (draw_rect.y + draw_rect.height > surface->height) draw_rect.height = surface->height - draw_rect.y;

    // Clip against explicit clip region
    if (clip) {
        BVRect dest;
        if (!dirty_region_clip(&dest, &draw_rect, clip)) {
            return; // completely clipped out
        }
        draw_rect = dest;
    }

    if (draw_rect.width <= 0 || draw_rect.height <= 0) return;

    for (int y = draw_rect.y; y < draw_rect.y + draw_rect.height; y++) {
        for (int x = draw_rect.x; x < draw_rect.x + draw_rect.width; x++) {
            surface->framebuffer[y * surface->width + x] = color;
        }
    }
}

void painter_draw_surface(struct BOSSurface* dest, struct BOSSurface* src, int dest_x, int dest_y, const BVRect* clip) {
    if (!dest || !dest->framebuffer || !src || !src->framebuffer || !src->visible) return;

    BVRect src_rect = {dest_x, dest_y, src->width, src->height};

    // Clip against dest bounds
    if (src_rect.x < 0) { src_rect.width += src_rect.x; src_rect.x = 0; }
    if (src_rect.y < 0) { src_rect.height += src_rect.y; src_rect.y = 0; }
    if (src_rect.x + src_rect.width > dest->width) src_rect.width = dest->width - src_rect.x;
    if (src_rect.y + src_rect.height > dest->height) src_rect.height = dest->height - src_rect.y;

    // Clip against explicit clip region
    if (clip) {
        BVRect clipped;
        if (!dirty_region_clip(&clipped, &src_rect, clip)) {
            return; // completely clipped out
        }
        src_rect = clipped;
    }

    if (src_rect.width <= 0 || src_rect.height <= 0) return;

    // Draw row by row
    for (int y = 0; y < src_rect.height; y++) {
        int d_y = src_rect.y + y;
        int s_y = (src_rect.y - dest_y) + y;
        
        for (int x = 0; x < src_rect.width; x++) {
            int d_x = src_rect.x + x;
            int s_x = (src_rect.x - dest_x) + x;
            
            BOVISUAL_Color pixel = src->framebuffer[s_y * src->width + s_x];
            
            // Simple alpha blending: if top byte (alpha) is 0, skip
            if ((pixel & 0xFF000000) != 0) {
                // In a real compositor, handle full alpha blending here
                dest->framebuffer[d_y * dest->width + d_x] = pixel;
            }
        }
    }
}
