#ifndef KERNEL_DISPATCHER_H
#define KERNEL_DISPATCHER_H

#include <stdint.h>
#include <stdbool.h>
#include "dispatcher_priority.h"
#include "dispatcher_queue.h"
#include "dispatcher_consumers.h"
#include "dispatcher_filters.h"
#include "dispatcher_router.h"
#include "dispatcher_diag.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 4: Universal Event Dispatcher Architecture
// ============================================================================
// The communication backbone between input producers and UI consumers.
// ============================================================================

// Initialize all Event Dispatcher sub-modules
void dispatcher_init(void);

// Push a standardized, immutable event into the dispatcher queue (O(1), interrupt-safe)
bool dispatcher_push_event(const DispatcherEvent* event);

// Pump and dispatch all queued events to registered consumers by priority order
void dispatcher_pump_events(void);

#endif // KERNEL_DISPATCHER_H
