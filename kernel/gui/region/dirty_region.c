#include "dirty_region.h"

// Simple math helpers
static inline int max(int a, int b) { return a > b ? a : b; }
static inline int min(int a, int b) { return a < b ? a : b; }

// Check if two rectangles intersect
static bool rect_intersect(const BVRect* a, const BVRect* b, BVRect* out) {
    int left = max(a->x, b->x);
    int top = max(a->y, b->y);
    int right = min(a->x + a->width, b->x + b->width);
    int bottom = min(a->y + a->height, b->y + b->height);

    if (left < right && top < bottom) {
        if (out) {
            out->x = left;
            out->y = top;
            out->width = right - left;
            out->height = bottom - top;
        }
        return true;
    }
    return false;
}

// Merge two rectangles into one bounding box
static void rect_union(BVRect* dest, const BVRect* a, const BVRect* b) {
    int left = min(a->x, b->x);
    int top = min(a->y, b->y);
    int right = max(a->x + a->width, b->x + b->width);
    int bottom = max(a->y + a->height, b->y + b->height);
    
    dest->x = left;
    dest->y = top;
    dest->width = right - left;
    dest->height = bottom - top;
}

void dirty_region_init(DirtyRegion* region) {
    if (!region) return;
    region->count = 0;
}

void dirty_region_add(DirtyRegion* region, const BVRect* rect) {
    if (!region || !rect || rect->width <= 0 || rect->height <= 0) return;

    // Optional: simple implementation that merges everything into one rect to avoid overflow
    // For a more advanced implementation, we would keep distinct rects and merge overlapping ones
    
    if (region->count == 0) {
        region->rects[0] = *rect;
        region->count = 1;
        return;
    }

    // Attempt to merge with existing rects if they intersect
    for (int i = 0; i < region->count; i++) {
        if (rect_intersect(&region->rects[i], rect, NULL)) {
            rect_union(&region->rects[i], &region->rects[i], rect);
            return;
        }
    }

    // If no intersection, add as new if there's room
    if (region->count < MAX_DIRTY_RECTS) {
        region->rects[region->count++] = *rect;
    } else {
        // Fallback: merge into the first rect (bounding box approach for everything)
        rect_union(&region->rects[0], &region->rects[0], rect);
    }
}

bool dirty_region_clip(BVRect* dest, const BVRect* src, const BVRect* clip_bounds) {
    if (!dest || !src || !clip_bounds) return false;
    return rect_intersect(src, clip_bounds, dest);
}

BVRect dirty_region_get_bounds(const DirtyRegion* region) {
    BVRect bounds = {0, 0, 0, 0};
    if (!region || region->count == 0) return bounds;

    bounds = region->rects[0];
    for (int i = 1; i < region->count; i++) {
        rect_union(&bounds, &bounds, &region->rects[i]);
    }
    return bounds;
}

void dirty_region_clear(DirtyRegion* region) {
    if (region) {
        region->count = 0;
    }
}

bool dirty_region_intersects(const DirtyRegion* region, const BVRect* rect) {
    if (!region || !rect) return false;
    for (int i = 0; i < region->count; i++) {
        if (rect_intersect(&region->rects[i], rect, NULL)) {
            return true;
        }
    }
    return false;
}
