#ifndef KERNEL_INPUT_ADAPTER_H
#define KERNEL_INPUT_ADAPTER_H

#include "input_core.h"
#include "kernel/drivers/input/input.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 2: Adapter Layer (Backward Compatibility)
// ============================================================================
// Bridges: Old Queue (input.c) -> Adapter Ingest -> Input Core -> Adapter Delivery -> Current BWE/Desktop
// ============================================================================

// Initialize the adapter layer and register as a Tier 1 consumer in Input Core
void input_adapter_init(void);

// Pump events from Legacy Ring Buffer 1 into Input Core and trigger dispatch
void input_adapter_pump(void);

// Register Adapter as a consumer of Phase 3 Pointer Engine broadcasts
void input_adapter_register_pointer_consumer(void);

#endif // KERNEL_INPUT_ADAPTER_H
