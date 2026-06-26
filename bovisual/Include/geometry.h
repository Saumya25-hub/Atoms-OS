#ifndef BOVISUAL_GEOMETRY_H
#define BOVISUAL_GEOMETRY_H

#include "bovisual_types.h"

// Math utilities
int32_t BV_Math_Abs(int32_t val);
int32_t BV_Math_Min(int32_t a, int32_t b);
int32_t BV_Math_Max(int32_t a, int32_t b);

// Geometry utilities
bool BV_RectContains(BVRect rect, BVPoint pt);
bool BV_RectIntersect(BVRect r1, BVRect r2);
BVRect BV_GetIntersection(BVRect r1, BVRect r2);
BVRect BV_RectInflate(BVRect r, int32_t dx, int32_t dy);
BVRect BV_RectDeflate(BVRect r, BVPadding p);
BVRect BV_RectOffset(BVRect r, int32_t dx, int32_t dy);

#endif // BOVISUAL_GEOMETRY_H
