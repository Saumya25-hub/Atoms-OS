#ifndef KERNEL_DISPATCHER_CONSUMERS_H
#define KERNEL_DISPATCHER_CONSUMERS_H

#include <stdint.h>
#include <stdbool.h>
#include "dispatcher_priority.h"
#include "dispatcher_queue.h"

#define DISPATCHER_MAX_CONSUMERS 32

typedef DispatchResult (*DispatcherCallback)(const DispatcherEvent* event, void* context);

typedef struct {
    uint32_t id;
    char name[32];
    DispatcherPriorityTier priority;
    uint32_t event_type_mask; // Bitmask of interested InputEventType (1 << type)
    bool enabled;
    void* context;
    DispatcherCallback callback;
} DispatcherConsumer;

// Initialize the consumer registry
void dispatcher_consumers_init(void);

// Register a consumer dynamically. Returns a unique consumer ID (> 0), or 0 on failure.
uint32_t dispatcher_consumers_register(const char* name, DispatcherPriorityTier priority,
                                       uint32_t event_mask, DispatcherCallback callback, void* context);

// Unregister a consumer by its ID. Returns true if removed.
bool dispatcher_consumers_unregister(uint32_t consumer_id);

// Enable or disable a consumer by its ID.
bool dispatcher_consumers_set_state(uint32_t consumer_id, bool enabled);

// Get the total number of registered consumers.
uint32_t dispatcher_consumers_get_count(void);

// Get read-only pointer to consumer by array index (for router priority traversal).
const DispatcherConsumer* dispatcher_consumers_get_by_index(uint32_t index);

#endif // KERNEL_DISPATCHER_CONSUMERS_H
