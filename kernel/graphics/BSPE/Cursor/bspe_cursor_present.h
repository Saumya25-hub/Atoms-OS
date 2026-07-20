#ifndef ATOMS_OS_BSPE_CURSOR_PRESENT_H
#define ATOMS_OS_BSPE_CURSOR_PRESENT_H

/**
 * @file bspe_cursor_present.h
 * @brief BSPE Asynchronous Cursor Presentation Layer (Phase 2)
 * @section PURPOSE
 * Decouples visual cursor presentation from the synchronous 60 Hz desktop window compositor.
 * Provides direct 4 KB VRAM damage blitting for 1000 Hz visual continuity in software mode
 * and instant MMIO register updates in hardware mode.
 */

#include <stdint.h>
#include <stdbool.h>
#include "bovisual/Include/bovisual_types.h"
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool is_initialized;
    bool visible;
    int32_t current_x;
    int32_t current_y;
    uint32_t width;
    uint32_t height;
    uint32_t hotspot_x;
    uint32_t hotspot_y;
    uint32_t scale_percent;
} BSPE_CursorPresenterState;

/**
 * @brief Initializes the asynchronous cursor presenter module.
 */
void BSPE_CursorPresenter_Init(void);

/**
 * @brief Asynchronously updates cursor position and blits directly to VRAM/RAM.
 * Executed at up to 1000 Hz from the Event Dispatcher / Cursor Engine.
 */
void BSPE_CursorPresenter_UpdatePosition(int32_t screen_x, int32_t screen_y, const uint32_t* bitmap, uint32_t width, uint32_t height, uint32_t hotspot_x, uint32_t hotspot_y, bool visible, uint32_t scale_percent);

/**
 * @brief Synchronizes cursor presentation after desktop window compositing.
 * Re-captures shadow background from newly composited windows and applies cursor sprite before VRAM swap.
 */
void BSPE_CursorPresenter_OnCompositorRedraw(const BVFramebuffer* ram_fb, const BVFramebuffer* hw_fb);

/**
 * @brief Restores the clean background under the cursor onto the target framebuffer.
 */
void BSPE_CursorPresenter_RestoreBackground(const BVFramebuffer* target_fb);

/**
 * @brief Phase 3: Synchronize with Compositor - Restore background and detach shadow before rendering.
 */
void BSPE_CursorPresenter_BeginComposition(void);

/**
 * @brief Phase 3: Synchronize with Compositor - Capture new background and re-attach shadow after rendering.
 */
void BSPE_CursorPresenter_EndComposition(void);

/**
 * @brief Phase 3: Fast-Path pump executed frequently to bypass Compositor.
 */
void BSPE_CursorPresenter_PumpFastPath(void);

/**
 * @brief Queries the current state of the cursor presenter.
 */
void BSPE_CursorPresenter_GetState(BSPE_CursorPresenterState* out_state);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_CURSOR_PRESENT_H
