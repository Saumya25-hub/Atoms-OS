#ifndef KERNEL_BOCOMPOSITOR_TYPES_H
#define KERNEL_BOCOMPOSITOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================
// BOCOMPOSITOR ENGINE v2 Configuration & Limits
// ============================================================
#define BOCOMPOSITOR_MAX_SURFACES 128
#define BOCOMPOSITOR_MAX_CLIPS    32
#define BOCOMPOSITOR_MAX_DAMAGE   32

// ============================================================
// Error Codes
// ============================================================
#define BOCOMPOSITOR_SUCCESS         0
#define BOCOMPOSITOR_ERR_INVALID_ID  1
#define BOCOMPOSITOR_ERR_POOL_FULL   2
#define BOCOMPOSITOR_ERR_NOT_FOUND   3

// ============================================================
// Geometry Structures
// ============================================================
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BOCompositorRect;

// ============================================================
// Surface States & Flags
// ============================================================
typedef enum {
    BOCOMPOSITOR_STATE_HIDDEN = 0,
    BOCOMPOSITOR_STATE_VISIBLE = 1,
    BOCOMPOSITOR_STATE_MINIMIZED = 2
} BOCompositorSurfaceState;

#define BOCOMPOSITOR_FLAG_VISIBLE    (1 << 0)
#define BOCOMPOSITOR_FLAG_FOCUSED    (1 << 1)
#define BOCOMPOSITOR_FLAG_ALWAYSTOP  (1 << 2)
#define BOCOMPOSITOR_FLAG_OPAQUE     (1 << 3)

// ============================================================
// Surface Representation
// ============================================================
typedef struct {
    uint32_t id;
    bool active;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t z_order;
    BOCompositorSurfaceState state;
    uint32_t flags;
    bool is_dirty;
    BOCompositorRect clip_rect;
    BOCompositorRect invalid_rect;
    void* surface_handle; // Opaque handle back to BOSurface
} BOCompositorSurface;

// ============================================================
// Debug Statistics
// ============================================================
typedef struct {
    uint32_t surface_count;
    uint32_t visible_windows;
    uint32_t hidden_windows;
    uint32_t dirty_rectangles;
    uint32_t clip_count;
    uint32_t occlusion_count;
    uint32_t batch_count;
    uint32_t compose_time_ms;
} BOCompositorStats;

#endif // KERNEL_BOCOMPOSITOR_TYPES_H
