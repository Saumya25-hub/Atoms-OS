#ifndef ATOMS_OS_BSPE_SWAPCHAIN_H
#define ATOMS_OS_BSPE_SWAPCHAIN_H

/**
 * @file swapchain.h
 * @brief BSPE Swapchain Public Header
 * @status Step 7 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public API for managing double and triple buffering rotation, buffer acquisition,
 * and atomic state transitions without heap allocation or VRAM copies.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Buffer State Enum --- */
typedef enum {
    BSPE_BUFFER_STATE_FREE               = 0,
    BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE   = 1,
    BSPE_BUFFER_STATE_QUEUED_IN_BSPE     = 2,
    BSPE_BUFFER_STATE_DISPLAYING_FRONT   = 3
} BSPE_BufferState;

/* --- Buffer & Configuration Structs --- */
typedef struct {
    uint32_t buffer_id;
    void* virtual_address;
    uint32_t physical_address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    BSPE_BufferState state;
    bool is_vram_page;
    uint32_t reserved[4]; /* Future extension reserve */
} BSPE_SwapchainBuffer;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t buffer_count; /* 2 = Double Buffering, 3 = Triple Buffering */
    uint32_t vsync_interval;
    uint32_t reserved[4]; /* Future extension reserve */
} BSPE_SwapchainConfig;

/* --- Public Lifecycle APIs --- */
BSPE_Error BSPE_Swapchain_Create(const BSPE_SwapchainConfig* config, BSPE_SwapchainHandle* out_handle);
void       BSPE_Swapchain_Destroy(BSPE_SwapchainHandle handle);
void       BSPE_Swapchain_Reset(BSPE_SwapchainHandle handle);
BSPE_Error BSPE_Swapchain_Resize(BSPE_SwapchainHandle handle, uint32_t new_width, uint32_t new_height);

/* --- Core Buffer State Machine Operations --- */
BSPE_Error BSPE_Swapchain_AcquireNextBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer);
BSPE_Error BSPE_Swapchain_Present(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer* buffer);
BSPE_Error BSPE_Swapchain_GetFrontBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer);

/* --- Aliases & Explicit APIs Required by Step 7 Specification --- */
static inline BSPE_Error BSPE_Swapchain_AcquireBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    return BSPE_Swapchain_AcquireNextBuffer(handle, out_buffer);
}
static inline BSPE_Error BSPE_Swapchain_QueueBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer* buffer) {
    return BSPE_Swapchain_Present(handle, buffer);
}
BSPE_Error BSPE_Swapchain_AcquireDisplayBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer);
BSPE_Error BSPE_Swapchain_ReleaseDisplayBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer* buffer);
static inline BSPE_Error BSPE_Swapchain_GetCurrentFront(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    return BSPE_Swapchain_GetFrontBuffer(handle, out_buffer);
}
BSPE_Error BSPE_Swapchain_GetCurrentBack(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer);

/* --- Verification & Diagnostics --- */
bool       BSPE_Swapchain_RunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_SWAPCHAIN_H
