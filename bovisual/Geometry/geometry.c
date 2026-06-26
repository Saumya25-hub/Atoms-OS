#include "../Include/geometry.h"

int32_t BV_Math_Abs(int32_t val) {
    return val < 0 ? -val : val;
}

int32_t BV_Math_Min(int32_t a, int32_t b) {
    return (a < b) ? a : b;
}

int32_t BV_Math_Max(int32_t a, int32_t b) {
    return (a > b) ? a : b;
}

bool BV_RectContains(BVRect rect, BVPoint pt) {
    return (pt.x >= rect.x && pt.x < rect.x + rect.width &&
            pt.y >= rect.y && pt.y < rect.y + rect.height);
}

bool BV_RectIntersect(BVRect r1, BVRect r2) {
    return !(r2.x >= r1.x + r1.width ||
             r2.x + r2.width <= r1.x ||
             r2.y >= r1.y + r1.height ||
             r2.y + r2.height <= r1.y);
}

BVRect BV_GetIntersection(BVRect r1, BVRect r2) {
    BVRect result = {0, 0, 0, 0};
    
    if (!BV_RectIntersect(r1, r2)) {
        return result;
    }

    int32_t max_x = BV_Math_Max(r1.x, r2.x);
    int32_t max_y = BV_Math_Max(r1.y, r2.y);
    int32_t min_right = BV_Math_Min(r1.x + r1.width, r2.x + r2.width);
    int32_t min_bottom = BV_Math_Min(r1.y + r1.height, r2.y + r2.height);

    result.x = max_x;
    result.y = max_y;
    result.width = min_right - max_x;
    result.height = min_bottom - max_y;

    return result;
}

BVRect BV_RectInflate(BVRect r, int32_t dx, int32_t dy) {
    r.x -= dx;
    r.y -= dy;
    r.width += dx * 2;
    r.height += dy * 2;
    return r;
}

BVRect BV_RectDeflate(BVRect r, BVPadding p) {
    r.x += p.left;
    r.y += p.top;
    r.width -= (p.left + p.right);
    r.height -= (p.top + p.bottom);
    if (r.width < 0) r.width = 0;
    if (r.height < 0) r.height = 0;
    return r;
}

BVRect BV_RectOffset(BVRect r, int32_t dx, int32_t dy) {
    r.x += dx;
    r.y += dy;
    return r;
}
