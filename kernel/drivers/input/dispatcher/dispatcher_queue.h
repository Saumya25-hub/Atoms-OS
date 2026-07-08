#ifndef KERNEL_DISPATCHER_QUEUE_H
#define KERNEL_DISPATCHER_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/drivers/input/core/input_core.h"

// Standardized immutable event payload for all ATOMS OS consumers
typedef InputCoreEvent DispatcherEvent;

#define DISPATCHER_QUEUE_SIZE 128 // Power of 2 for fast modulo indexing

// Initialize the event dispatcher FIFO ring buffer
void dispatcher_queue_init(void);

// Push an immutable event into the queue (interrupt-safe, lock-free)
// Returns true if successfully queued, false if queue overflowed
bool dispatcher_queue_push(const DispatcherEvent* event);

// Pop the oldest event from the queue
// Returns true if an event was popped, false if queue is empty
bool dispatcher_queue_pop(DispatcherEvent* out_event);

// Check if queue is empty
bool dispatcher_queue_is_empty(void);

// Check if queue is full
bool dispatcher_queue_is_full(void);

// Get current number of queued events
uint32_t dispatcher_queue_get_depth(void);

// Clear all events from the queue
void dispatcher_queue_clear(void);

#endif // KERNEL_DISPATCHER_QUEUE_H
