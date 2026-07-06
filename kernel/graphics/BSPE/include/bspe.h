#ifndef ATOMS_OS_BSPE_H
#define ATOMS_OS_BSPE_H

/**
 * @file bspe.h
 * @brief BOS Surface Presentation Engine (BSPE) Master Public Header
 * @status Step 3 API Freeze (Zero Implementation Logic)
 * 
 * @section PURPOSE
 * Master include header for the BOS Surface Presentation Engine (BSPE).
 * Defines core error codes, version constants, opaque handles, and lifecycle APIs.
 * 
 * @section DEPENDENCY_LIST
 * - <stdint.h>, <stdbool.h>
 * - "../../BOGE/include/boge.h" (for BOGE_StagingFrame, BOGE_Rect forward declarations)
 * - Zero circular includes. Zero video driver dependencies.
 * 
 * @section MEMORY_OWNERSHIP
 * - BSPE_Config pointers passed to BSPE_Initialize are caller-owned and borrowed for the call duration.
 * - All internal engine state is allocated from system kernel slabs during BSPE_Initialize and owned exclusively by BSPE until BSPE_Shutdown.
 * 
 * @section THREAD_OWNERSHIP
 * - BSPE_Initialize and BSPE_Shutdown must be called strictly from the Master Kernel Initialization Thread.
 * - BSPE_PresentFrame is thread-safe and lock-free when called from the BOGE V2 Compositor Thread.
 * 
 * @section LIFETIME_RULES
 * - Engine must be initialized via BSPE_Initialize before any subsystem handles are created.
 * - Engine shutdown via BSPE_Shutdown invalidates all outstanding BSPE handles.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../BOGE/include/boge.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Version Information --- */
#define BSPE_VERSION_MAJOR 2
#define BSPE_VERSION_MINOR 0
#define BSPE_VERSION_PATCH 0

/* --- Error Codes --- */
typedef enum {
    BSPE_OK                   =  0,
    BSPE_ERR_NULL_POINTER     = -1,
    BSPE_ERR_OUT_OF_MEMORY    = -2,
    BSPE_ERR_QUEUE_FULL       = -3,
    BSPE_ERR_QUEUE_EMPTY      = -4,
    BSPE_ERR_DRIVER_NOT_FOUND = -5,
    BSPE_ERR_INVALID_STATE    = -6,
    BSPE_ERR_UNSUPPORTED      = -7,
    BSPE_ERR_TIMEOUT          = -8
} BSPE_Error;

/* --- Opaque Handle Definitions --- */
typedef struct BSPE_PresentQueue_T*   BSPE_PresentQueueHandle;
typedef struct BSPE_Swapchain_T*      BSPE_SwapchainHandle;
typedef struct BSPE_DamageTracker_T*  BSPE_DamageTrackerHandle;
typedef struct BSPE_FramePacer_T*     BSPE_FramePacerHandle;
typedef struct BSPE_CursorPlane_T*    BSPE_CursorPlaneHandle;
typedef struct BSPE_DisplayDriver_T*  BSPE_DisplayDriverHandle;
typedef struct BSPE_TelemetryHUD_T*   BSPE_TelemetryHUDHandle;

/* --- Core Configuration Struct --- */
typedef struct {
    uint32_t display_width;
    uint32_t display_height;
    uint32_t buffer_count;
    bool enable_vsync;
    uint32_t reserved[4]; /* Future extension reserve */
} BSPE_Config;

/* --- Public Lifecycle & Presentation APIs --- */
/**
 * @brief Initializes the global BSPE presentation engine.
 * @param config Pointer to caller-owned configuration struct.
 * @return BSPE_OK on success; negative error code on failure.
 */
BSPE_Error BSPE_Initialize(const BSPE_Config* config);

/**
 * @brief Shuts down BSPE and reclaims all kernel memory allocations.
 */
void BSPE_Shutdown(void);

/**
 * @brief Submits a completed staging frame from BOGE V2 for presentation.
 * @param frame Pointer to caller-owned staging frame.
 * @return BSPE_OK on success; BSPE_ERR_QUEUE_FULL if pacing queue is saturated.
 */
BSPE_Error BSPE_PresentFrame(const BOGE_StagingFrame* frame);

/**
 * @brief Sets the presentation synchronization interval.
 * @param swap_interval 1 = VSync 60Hz/144Hz; 0 = Immediate (Tearing allowed).
 */
void BSPE_SetSwapInterval(uint32_t swap_interval);

/* --- Cursor Integration --- */
extern bool g_bspe_use_hardware_cursor;
BSPE_Error BSPE_SetCursorPosition(int32_t x, int32_t y);
bool BSPE_IsHardwareCursorActive(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_H
