#ifndef KERNEL_DISPATCHER_ROUTER_H
#define KERNEL_DISPATCHER_ROUTER_H

#include "dispatcher_queue.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 4: Event Dispatcher Routing Engine
// ============================================================================
// Executes deterministic priority traversal and propagation control.
// ============================================================================

// Initialize router module
void dispatcher_router_init(void);

// Route an immutable event through filters and registered consumers by priority order.
void dispatcher_router_route(const DispatcherEvent* event);

#endif // KERNEL_DISPATCHER_ROUTER_H
