#ifndef BOSFSR_GRAPHICS_H
#define BOSFSR_GRAPHICS_H

#include <stdint.h>
#include "kernel/wm/bwe/include/bwe.h"

void bosfsr_draw_rounded_rect(const BVFramebuffer* fb, int x, int y, int w, int h, int radius, uint32_t fill_color, uint32_t border_color, int border_w);
void bosfsr_draw_linear_gradient(const BVFramebuffer* fb, int x, int y, int w, int h, uint32_t start_col, uint32_t end_col, bool vertical);
void bosfsr_draw_shadow(const BVFramebuffer* fb, int x, int y, int w, int h, int radius);

#endif /* BOSFSR_GRAPHICS_H */
