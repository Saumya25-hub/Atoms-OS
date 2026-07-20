#ifndef BWE_GEOMETRY_H
#define BWE_GEOMETRY_H

#include "bwe.h"

// Calculate the screen bounds of the window (including decorations)
void BWE_Geometry_CalculateScreenBounds(BWE_Window* win, BWE_Rect* out_bounds);

// Calculate the inner client area screen bounds (excluding decorations)
void BWE_Geometry_CalculateClientBounds(BWE_Window* win, BWE_Rect* out_bounds);

// Helper to get decoration offsets for a given window
void BWE_Geometry_GetDecorationMetrics(BWE_Window* win, int32_t* top, int32_t* bottom, int32_t* left, int32_t* right);

#endif // BWE_GEOMETRY_H
