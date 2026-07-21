#ifndef BRE_H
#define BRE_H

#include "kernel/core/brtsl/bre_types.h"
#include "kernel/core/brtsl/bre_telemetry.h"

// Initialize the BOS Reflex Engine
void BRE_Init(void);

// Register a service callback with a specific budget
void BRE_RegisterService(BreServiceId id, BreServiceCallback callback, uint32_t default_budget);

// Disable a service (fallback to legacy behavior if needed)
void BRE_DisableService(BreServiceId id);

// Signal that a service has pending work. O(1), Non-blocking.
void BRE_Signal(BreServiceId id);

// Dispatcher: executes pending services up to their budget.
// Called from timer IRQ tail or cooperative yield.
void BRE_DispatchPending(void);

#endif // BRE_H
