#ifndef BOSURFACE_SURFACE_H
#define BOSURFACE_SURFACE_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// BWE Error Codes (from BWE_API_SPECIFICATION.md)
// ============================================================
#define BWE_SUCCESS   0x0000
#define BWE0001       0x1001  // Invalid Surface Pointer / ID
#define BWE0002       0x1002  // Invalid Parent ID
#define BWE0003       0x1003  // Render Failed
#define BWE0004       0x1004  // Memory Allocation Failed
#define BWE0005       0x1005  // Focus Error
#define BWE0006       0x1006  // Dirty Region Overflow
#define BWE0007       0x1007  // Invalid Control ID
#define BWE0008       0x1008  // Surface Already Exists

// ============================================================
// BWE Surface Flags
// ============================================================
#define BWE_FLAG_VISIBLE        (1 << 0)
#define BWE_FLAG_FOCUSED        (1 << 1)
#define BWE_FLAG_ALPHA          (1 << 2)
#define BWE_FLAG_DOUBLEBUFFERED (1 << 3)
#define BWE_FLAG_DRAGGABLE      (1 << 4)

// ============================================================
// BWE Surface States (Lifecycle)
// ============================================================
typedef enum {
    BWE_STATE_CREATED,
    BWE_STATE_VISIBLE,
    BWE_STATE_HIDDEN,
    BWE_STATE_FOCUSED,
    BWE_STATE_INACTIVE,
    BWE_STATE_MINIMIZED,
    BWE_STATE_MAXIMIZED,
    BWE_STATE_DESTROYED
} BWE_SurfaceState;

// ============================================================
// BWE Surface Geometry (Absolute Screen Coordinates)
// ============================================================
typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} BWE_Rect;

// ============================================================
// BWE Surface Structure
// ============================================================
#define BWE_MAX_SURFACES  64
#define BWE_MAX_CHILDREN  16
#define BWE_DESKTOP_ID    0

typedef struct BWE_Surface {
    // Identity
    uint32_t            id;
    bool                active;        // Is this slot in use?

    // Tree: Parent-Child Relationships
    uint32_t            parent_id;
    uint32_t            children[BWE_MAX_CHILDREN];
    uint32_t            child_count;
    uint32_t            z_order;       // Higher = drawn later (on top)

    // Geometry (relative to parent)
    BWE_Rect            local_bounds;
    // Geometry (absolute screen coordinates, computed by compositor)
    BWE_Rect            screen_bounds;

    // State
    BWE_SurfaceState    state;
    uint32_t            flags;

    // Owner (for future process tracking)
    uint32_t            owner_pid;
} BWE_Surface;

typedef uint32_t bwe_error_t;

#include "bovisual/Include/events.h"

// ============================================================
// BWE Public API (from BWE_API_SPECIFICATION.md)
// ============================================================

// Core Lifecycle
void        BOSurface_Init(void);
bwe_error_t BOS_CreateSurface(uint32_t parent_id, uint32_t x, uint32_t y,
                               uint32_t width, uint32_t height,
                               uint32_t flags, uint32_t* out_surface_id);
bwe_error_t BOS_DestroySurface(uint32_t surface_id);

// Visibility
bwe_error_t BOS_Show(uint32_t target_id);
bwe_error_t BOS_Hide(uint32_t target_id);

// Geometry
bwe_error_t BOS_SetBounds(uint32_t target_id, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height);

// Focus Engine
bwe_error_t BOS_SetFocus(uint32_t surface_id);
uint32_t    BOS_GetFocus(void);
bwe_error_t BOS_ClearFocus(void);
uint32_t    BOS_GetActiveSurface(void);

// Events & Interaction
uint32_t    BWE_HitTest(int32_t screen_x, int32_t screen_y);
void        BOS_ProcessEvent(const BVEvent* event);

// Compositor
void        BWE_ComputeScreenBounds(void);
void        BWE_Compose(void);

// Accessors
BWE_Surface* BWE_GetSurface(uint32_t surface_id);
uint32_t     BWE_GetSurfaceCount(void);

// Phase Tests
void BOS_Test_Phase1(void);
void BOS_Test_Phase2(void);
void BOS_Test_Phase3(void);
void BOS_Test_Phase4(void);

#endif // BOSURFACE_SURFACE_H
