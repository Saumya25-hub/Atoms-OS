#ifndef BOSCAL_TYPES_H
#define BOSCAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// ----------------------------------------------------
// BOSCAL: Responsive Dimension Units
// ----------------------------------------------------
typedef enum {
    BV_UNIT_PX,      // Absolute pixels (Legacy/Fixed)
    BV_UNIT_PERCENT, // Percentage of Parent (0-100)
    BV_UNIT_VW,      // Percentage of Viewport Width (0-100)
    BV_UNIT_VH,      // Percentage of Viewport Height (0-100)
    BV_UNIT_AUTO,    // Hug Content (e.g. text size)
    BV_UNIT_FILL     // Fill available remaining space
} BVUnitType;

typedef struct {
    int32_t value;
    BVUnitType type;
} BVDimension;

// ----------------------------------------------------
// BOSCAL: Anchoring System
// ----------------------------------------------------
typedef enum {
    BV_ANCHOR_NONE,     // Manual XY (Legacy)
    BV_ANCHOR_TOP,      // Dock Top
    BV_ANCHOR_BOTTOM,   // Dock Bottom
    BV_ANCHOR_LEFT,     // Dock Left
    BV_ANCHOR_RIGHT,    // Dock Right
    BV_ANCHOR_CENTER,   // Center exactly in parent
    BV_ANCHOR_STRETCH,  // Stretch width but fixed height (or vice versa)
    BV_ANCHOR_FILL      // Fill entirely
} BVAnchor;

// Helper Macros
#define BV_PX(v)      (BVDimension){ (v), BV_UNIT_PX }
#define BV_PCT(v)     (BVDimension){ (v), BV_UNIT_PERCENT }
#define BV_VW(v)      (BVDimension){ (v), BV_UNIT_VW }
#define BV_VH(v)      (BVDimension){ (v), BV_UNIT_VH }
#define BV_AUTO()     (BVDimension){ 0, BV_UNIT_AUTO }
#define BV_FILL()     (BVDimension){ 0, BV_UNIT_FILL }

#endif // BOSCAL_TYPES_H
