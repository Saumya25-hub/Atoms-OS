#ifndef ATOMS_OS_INPUT_CURSOR_RENDERER_H
#define ATOMS_OS_INPUT_CURSOR_RENDERER_H

/**
 * @file cursor_renderer.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Rendering Pipeline
 * @section PURPOSE
 * Deterministic visual rendering pipeline. Coordinates hardware cursor plane updates
 * and software fallback sprite rendering with shadow buffer background restoration
 * and dirty rectangle damage tracking.
 */

#include <stdint.h>
#include <stdbool.h>
#include "cursor_hotspot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Lifecycle & Initialization --- */
void cursor_renderer_init(void);

/* --- Pipeline Update Trigger --- */
/**
 * @brief Invoked when pointer kinematics or theme/animation state change.
 * Coordinates with backend without modifying pointer kinematics.
 */
void cursor_renderer_update(bool moved);

/* --- Software Fallback Rendering & Dirty Region Optimization --- */
/**
 * @brief Renders the active cursor sprite onto a framebuffer with background
 * restoration and damage tracking.
 * @param fb_ptr Pointer to BVFramebuffer struct.
 */
void cursor_renderer_draw_software(const void* fb_ptr);

/**
 * @brief Restores background pixels under the previous cursor bounding box.
 * @param fb_ptr Pointer to BVFramebuffer struct.
 */
void cursor_renderer_restore_background(const void* fb_ptr);

/* --- Dirty Region Query --- */
void cursor_renderer_get_dirty_box(CursorBoundingBox* out_box);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_RENDERER_H
