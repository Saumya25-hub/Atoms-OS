#ifndef BRE_TYPES_H
#define BRE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Predefined Service IDs for the BOS Reflex Engine
typedef enum {
    BRE_SERVICE_AUDIO = 0,
    BRE_SERVICE_MAX = 32 // Maximum number of services (fits in a 32-bit bitmask)
} BreServiceId;

/**
 * @brief BRE Service Callback
 * 
 * @param budget The maximum number of work units this service is allowed to process.
 *               The unit is defined by the service itself (e.g., DMA descriptors for audio).
 * @return bool  Returns true if there is STILL MORE WORK pending after exhausting the budget.
 *               Returns false if all pending work was completed within the budget.
 */
typedef bool (*BreServiceCallback)(uint32_t budget);

#endif // BRE_TYPES_H
