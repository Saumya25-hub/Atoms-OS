#ifndef BOVISUAL_LAYOUT_H
#define BOVISUAL_LAYOUT_H

#include "geometry.h"
#include "text.h"

typedef enum {
    BV_ALIGN_START,    // Left or Top
    BV_ALIGN_CENTER,   // Center
    BV_ALIGN_END       // Right or Bottom
} BVLayoutAlignment;

typedef struct {
    BVRect content_bounds;
    BVRect text_bounds;
} BVLayoutResult;

// Layout Engine Engine
BVLayoutResult BV_CalculateLayout(BVRect bounds, BVPadding padding, BVTextMetrics textMetrics, BVLayoutAlignment hAlign, BVLayoutAlignment vAlign);

#endif // BOVISUAL_LAYOUT_H
