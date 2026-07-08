#ifndef ATOMS_OS_INPUT_CURSOR_ENGINE_H
#define ATOMS_OS_INPUT_CURSOR_ENGINE_H

/**
 * @file cursor_engine.h
 * @brief ATOMS OS Input Engine V2 - Phase 5 Cursor Engine Coordinator
 * @section PURPOSE
 * Master coordinator for the Cursor Engine subsystem. Registers as Tier 1 consumer
 * in the Event Dispatcher, coordinates state/theme/animation/backend modules, and
 * provides the single authoritative visual presentation interface for ATOMS OS.
 */

#include <stdint.h>
#include <stdbool.h>
#include "kernel/drivers/input/dispatcher/dispatcher_consumers.h"
#include "cursor_state.h"
#include "cursor_theme.h"
#include "cursor_animation.h"
#include "cursor_hotspot.h"
#include "cursor_backend.h"
#include "cursor_renderer.h"
#include "cursor_diag.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Lifecycle & Initialization --- */
void cursor_engine_init(uint32_t screen_width, uint32_t screen_height);
void cursor_engine_update_resolution(uint32_t screen_width, uint32_t screen_height);

/* --- Tier 1 Event Dispatcher Consumer Callback --- */
/**
 * @brief Authoritative Tier 1 consumer callback receiving post-filtered kinematics.
 * Updates presentation coordinates and triggers rendering without consuming events.
 */
DispatchResult cursor_engine_dispatch_cb(const DispatcherEvent* event, void* context);

/* --- Compositor Integration Hook --- */
/**
 * @brief Authoritative presentation hook invoked by desktop and window compositors.
 * Renders software sprite overlay if hardware cursor mode is inactive.
 * @param fb_ptr Pointer to BVFramebuffer struct.
 */
void cursor_engine_render_overlay(const void* fb_ptr);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_INPUT_CURSOR_ENGINE_H
