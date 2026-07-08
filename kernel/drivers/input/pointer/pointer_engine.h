#ifndef KERNEL_POINTER_ENGINE_H
#define KERNEL_POINTER_ENGINE_H

#include <stdint.h>
#include "kernel/drivers/input/core/input_core.h"
#include "pointer_state.h"
#include "pointer_precision.h"
#include "pointer_velocity.h"
#include "pointer_buttons.h"
#include "pointer_bounds.h"
#include "pointer_consumers.h"
#include "pointer_motion.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Pointer Engine Architecture
// ============================================================================
// The single authoritative source of all pointer state inside ATOMS OS.
// Sits immediately after the Input Core as a Tier 0 Consumer.
// ============================================================================

// Initialize all Pointer Engine sub-modules and register with Input Core
void pointer_engine_init(uint32_t screen_width, uint32_t screen_height);

// Tier 0 Consumer callback registered in Input Core
bool pointer_engine_on_event(const InputCoreEvent* event, void* user_data);

#endif // KERNEL_POINTER_ENGINE_H
