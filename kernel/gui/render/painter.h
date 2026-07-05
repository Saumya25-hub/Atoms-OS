#ifndef GUI_PAINTER_H
#define GUI_PAINTER_H

#include "kernel/gui/surface/surface.h"
#include "kernel/gui/region/dirty_region.h"
#include "bovisual/Include/bovisual_types.h"
#include <stdint.h>

// Painter abstraction for drawing to a specific surface or framebuffer
// Respects clip bounds if provided.

void painter_draw_pixel(struct BOSSurface* surface, int x, int y, BOVISUAL_Color color, const BVRect* clip);
void painter_draw_line(struct BOSSurface* surface, int x0, int y0, int x1, int y1, BOVISUAL_Color color, const BVRect* clip);
void painter_draw_rect(struct BOSSurface* surface, const BVRect* rect, BOVISUAL_Color color, const BVRect* clip);
void painter_fill_rect(struct BOSSurface* surface, const BVRect* rect, BOVISUAL_Color color, const BVRect* clip);

// Draw one surface onto another (composition)
void painter_draw_surface(struct BOSSurface* dest, struct BOSSurface* src, int dest_x, int dest_y, const BVRect* clip);

// Optional primitives
// void painter_draw_text(struct BOSSurface* surface, int x, int y, const char* text, BOVISUAL_Color color, const BVRect* clip);
// void painter_draw_bitmap(struct BOSSurface* surface, int x, int y, uint32_t* bitmap, int w, int h, const BVRect* clip);

#endif // GUI_PAINTER_H
