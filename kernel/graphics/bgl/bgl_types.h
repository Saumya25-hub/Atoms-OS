#ifndef BGL_TYPES_H
#define BGL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================
// BGL Error Codes
// ============================================================
typedef enum {
    BGL_SUCCESS                 = 0,
    BGL_ERR_INVALID_CONTEXT     = 1,
    BGL_ERR_INVALID_DRAWABLE    = 2,
    BGL_ERR_OUT_OF_MEMORY       = 3,
    BGL_ERR_ALREADY_CURRENT     = 4,
    BGL_ERR_NOT_CURRENT         = 5,
    BGL_ERR_PERMISSION_DENIED   = 6,
    BGL_ERR_INVALID_DIMENSIONS  = 7,
    BGL_ERR_DRAWABLE_BUSY       = 8,
    BGL_ERR_SURFACE_NOT_FOUND   = 9,
    BGL_ERR_STATE_CORRUPT       = 10
} BGLError;

// ============================================================
// BGL Context State Enum
// ============================================================
typedef enum {
    BGL_STATE_CREATED           = 0,
    BGL_STATE_CURRENT           = 1,
    BGL_STATE_DETACHED          = 2,
    BGL_STATE_DESTROY_PENDING   = 3,
    BGL_STATE_DESTROYED         = 4
} BGLContextState;

// ============================================================
// BGL Pixel Formats
// ============================================================
typedef enum {
    BGL_FORMAT_ARGB8888         = 1
} BGLPixelFormat;

#define BGL_MAX_CONTEXTS  64
#define BGL_MAX_DRAWABLES 64

#endif // BGL_TYPES_H
