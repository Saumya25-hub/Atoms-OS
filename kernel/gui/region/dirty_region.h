#ifndef GUI_DIRTY_REGION_H
#define GUI_DIRTY_REGION_H

#include <stdint.h>
#include <stdbool.h>
#include "bovisual/Include/bovisual_types.h" // For BVRect

#define MAX_DIRTY_RECTS 32

typedef struct {
    BVRect rects[MAX_DIRTY_RECTS];
    int count;
} DirtyRegion;

// Initialize a dirty region tracker
void dirty_region_init(DirtyRegion* region);

// Add a new rectangle to the dirty region, merging if necessary
void dirty_region_add(DirtyRegion* region, const BVRect* rect);

// Clip a rendering rectangle against the screen bounds
bool dirty_region_clip(BVRect* dest, const BVRect* src, const BVRect* clip_bounds);

// Get the merged bounding box of all dirty regions
BVRect dirty_region_get_bounds(const DirtyRegion* region);

// Clear the dirty region tracker
void dirty_region_clear(DirtyRegion* region);

// Check if a given rect intersects with any dirty area
bool dirty_region_intersects(const DirtyRegion* region, const BVRect* rect);

#endif // GUI_DIRTY_REGION_H
