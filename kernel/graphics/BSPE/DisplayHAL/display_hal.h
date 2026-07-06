#ifndef ATOMS_OS_BSPE_DISPLAY_HAL_H
#define ATOMS_OS_BSPE_DISPLAY_HAL_H

/**
 * @file display_hal.h
 * @brief BSPE Display Hardware Abstraction Layer Public Header
 * @status Step 4 Production Implementation
 * 
 * @section PURPOSE
 * Defines the standardized interface table (BSPE_DisplayDriverInterface) and delegation APIs
 * that decouple BSPE presentation logic from physical Bochs VBE, VGA, VESA, and GPU hardware drivers.
 * 
 * @section DEPENDENCY_LIST
 * - "../include/bspe.h" (for BSPE_Error, BSPE_DisplayDriverHandle)
 * - Zero circular includes. Zero hardware driver includes.
 * 
 * @section MEMORY_OWNERSHIP
 * - BSPE_DisplayDriverInterface struct pointers registered are static or caller-owned immutable tables.
 * - Driver handle is managed by DisplayHAL and remains valid until unregistered.
 * 
 * @section THREAD_OWNERSHIP
 * - RegisterDriver and SetActiveDriver must be called from the Kernel Initialization Thread.
 * - HAL function pointers (swap_page, copy_rect_to_vram) are invoked by the BSPE Presentation Thread.
 * 
 * @section LIFETIME_RULES
 * - Active driver remains bound until explicitly unregistered or replaced.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Driver Capabilities Struct --- */
typedef struct {
    uint32_t max_width;
    uint32_t max_height;
    uint32_t vram_size_bytes;
    bool supports_hw_cursor;
    bool supports_page_flip;
    uint32_t reserved[4]; /* Future extension reserve */
} BSPE_DisplayDriverCaps;

/* --- Hardware Driver Interface Table --- */
typedef struct {
    BSPE_Error (*init)(uint32_t width, uint32_t height, uint32_t bpp);
    void       (*shutdown)(void);
    void*      (*get_framebuffer_base)(void);
    BSPE_Error (*swap_page)(uint32_t y_offset);
    void       (*wait_vsync)(void);
    BSPE_Error (*copy_rect_to_vram)(const void* src_ram, uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height);
    BSPE_Error (*cursor_set_position)(int32_t x, int32_t y);
    BSPE_Error (*cursor_set_image)(const uint32_t* argb_32x32);
    void       (*cursor_enable)(bool enable);
    void*      reserved_ptrs[4]; /* Future extension function pointers */
} BSPE_DisplayDriverInterface;

/* --- Public Display HAL Management APIs --- */
BSPE_Error BSPE_DisplayHAL_RegisterDriver(const char* name, const BSPE_DisplayDriverInterface* interface, BSPE_DisplayDriverHandle* out_handle);
void BSPE_DisplayHAL_UnregisterDriver(BSPE_DisplayDriverHandle handle);
BSPE_Error BSPE_DisplayHAL_SetActiveDriver(BSPE_DisplayDriverHandle handle);
BSPE_DisplayDriverHandle BSPE_DisplayHAL_GetActiveDriver(void);
BSPE_Error BSPE_DisplayHAL_GetCaps(BSPE_DisplayDriverHandle handle, BSPE_DisplayDriverCaps* out_caps);
BSPE_Error BSPE_DisplayHAL_ProbeAndSelectDriver(void);

/* --- High-Level HAL Delegation APIs (Invoking Active Driver Backend) --- */
BSPE_Error BSPE_DisplayHAL_InitDisplay(uint32_t width, uint32_t height, uint32_t bpp);
void       BSPE_DisplayHAL_ShutdownDisplay(void);
void*      BSPE_DisplayHAL_GetFramebufferBase(void);
BSPE_Error BSPE_DisplayHAL_SwapPage(uint32_t y_offset);
void       BSPE_DisplayHAL_WaitForVSync(void);
BSPE_Error BSPE_DisplayHAL_CopyRectToVRAM(const void* src_ram, uint32_t dest_x, uint32_t dest_y, uint32_t width, uint32_t height);
BSPE_Error BSPE_DisplayHAL_CursorSetPosition(int32_t x, int32_t y);
BSPE_Error BSPE_DisplayHAL_CursorSetImage(const uint32_t* argb_32x32);
void       BSPE_DisplayHAL_CursorEnable(bool enable);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_DISPLAY_HAL_H
