#ifndef KERNEL_DISPATCHER_FILTERS_H
#define KERNEL_DISPATCHER_FILTERS_H

#include <stdint.h>
#include <stdbool.h>
#include "dispatcher_queue.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 4: Event Dispatcher Filtering Pipeline
// ============================================================================
// Modular filters suppressing duplicates, zero-delta noise, and error packets.
// ============================================================================

// Initialize filtering module state
void dispatcher_filters_init(void);

// Evaluate an immutable event against active filter pipeline.
// Returns true if the event should be PASSED to consumers.
// Returns false if the event should be DROPPED (and logs diagnostic drop).
bool dispatcher_filters_evaluate(const DispatcherEvent* event);

#endif // KERNEL_DISPATCHER_FILTERS_H
