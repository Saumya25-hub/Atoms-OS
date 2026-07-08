#ifndef KERNEL_POINTER_MOTION_H
#define KERNEL_POINTER_MOTION_H

#include "kernel/drivers/input/core/input_core.h"
#include "pointer_state.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: 7-Stage Motion Processing Pipeline
// ============================================================================

// Initialize motion processing pipeline
void pointer_motion_init(void);

// Execute deterministic 7-stage motion pipeline on an incoming InputCoreEvent
void pointer_motion_process(const InputCoreEvent* event);

#endif // KERNEL_POINTER_MOTION_H
