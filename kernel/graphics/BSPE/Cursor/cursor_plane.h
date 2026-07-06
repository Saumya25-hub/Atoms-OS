#ifndef ATOMS_OS_BSPE_CURSOR_PLANE_H
#define ATOMS_OS_BSPE_CURSOR_PLANE_H

/**
 * @file cursor_plane.h
 * @brief BSPE Hardware Cursor Plane Public Header
 * @status Step 9 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public API for asynchronous hardware cursor plane control and software fallback abstraction.
 * Decouples mouse movement from the compositor rendering pipeline without VRAM copies or heap allocation.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Cursor Mode Enum --- */
typedef enum {
    BSPE_CURSOR_MODE_NONE     = 0,
    BSPE_CURSOR_MODE_HARDWARE = 1,
    BSPE_CURSOR_MODE_SOFTWARE = 2
} BSPE_CursorMode;

/* --- Configuration Struct --- */
typedef struct {
    uint32_t max_width;               /* Sprite width (default 64) */
    uint32_t max_height;              /* Sprite height (default 64) */
    bool allow_software_fallback;     /* Enable async fallback if HW registers fail */
    bool force_software_mode;         /* For testing software fallback path */
    uint32_t reserved[3];             /* Future extension reserve */
} BSPE_CursorPlaneConfig;

/* --- Public Lifecycle APIs --- */
BSPE_Error BSPE_CursorPlane_Create(const BSPE_CursorPlaneConfig* config, BSPE_CursorPlaneHandle* out_handle);
void       BSPE_CursorPlane_Destroy(BSPE_CursorPlaneHandle handle);

/* --- Aliases & Explicit APIs Required by Step 9 Specification --- */
static inline BSPE_Error BSPE_CursorPlane_Initialize(const BSPE_CursorPlaneConfig* config, BSPE_CursorPlaneHandle* out_handle) {
    return BSPE_CursorPlane_Create(config, out_handle);
}
BSPE_Error BSPE_CursorPlane_SetMode(BSPE_CursorPlaneHandle handle, BSPE_CursorMode mode);
BSPE_Error BSPE_CursorPlane_SetPosition(BSPE_CursorPlaneHandle handle, int32_t x, int32_t y);
BSPE_Error BSPE_CursorPlane_SetImage(BSPE_CursorPlaneHandle handle, const uint32_t* argb_bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y);
BSPE_Error BSPE_CursorPlane_SetVisibility(BSPE_CursorPlaneHandle handle, bool visible);
BSPE_Error BSPE_CursorPlane_Show(BSPE_CursorPlaneHandle handle);
BSPE_Error BSPE_CursorPlane_Hide(BSPE_CursorPlaneHandle handle);
BSPE_Error BSPE_CursorPlane_GetPosition(BSPE_CursorPlaneHandle handle, int32_t* out_x, int32_t* out_y);
BSPE_Error BSPE_CursorPlane_GetState(BSPE_CursorPlaneHandle handle, bool* out_visible, BSPE_CursorMode* out_mode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_hotspot_x, uint32_t* out_hotspot_y);

/* --- Hardware Query from Step 3 --- */
bool       BSPE_CursorPlane_IsHardwareSupported(BSPE_CursorPlaneHandle handle);

/* --- Verification & Diagnostics --- */
bool       BSPE_CursorPlane_RunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_CURSOR_PLANE_H
