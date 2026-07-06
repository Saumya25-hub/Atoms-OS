#ifndef ATOMS_OS_BSPE_PRESENT_QUEUE_H
#define ATOMS_OS_BSPE_PRESENT_QUEUE_H

/**
 * @file present_queue.h
 * @brief BSPE Present Queue Public Header
 * @status Step 5 Production Implementation
 * 
 * @section PURPOSE
 * Defines the public API for the asynchronous lock-free staging frame ring buffer.
 * Decouples BOGE V2 compositing loops from physical VSync presentation timing.
 */

#include <stdint.h>
#include <stdbool.h>
#include "../include/bspe.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Configuration & Statistics Structs --- */
typedef struct {
    uint32_t capacity;      /* Maximum number of queued frames (power of 2, max 16) */
    uint32_t reserved[4];   /* Future extension reserve */
} BSPE_PresentQueueConfig;

typedef struct {
    uint32_t total_enqueued;
    uint32_t total_dequeued;
    uint32_t dropped_frames;
    uint32_t current_depth;
    uint32_t reserved[4];   /* Future extension reserve */
} BSPE_PresentQueueStats;

/* --- Public Present Queue Lifecycle APIs --- */
BSPE_Error BSPE_PresentQueue_Create(const BSPE_PresentQueueConfig* config, BSPE_PresentQueueHandle* out_handle);
void       BSPE_PresentQueue_Destroy(BSPE_PresentQueueHandle handle);
void       BSPE_PresentQueue_Reset(BSPE_PresentQueueHandle handle);

/* --- Core SPSC Ring Buffer Operations --- */
BSPE_Error BSPE_PresentQueue_Enqueue(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame* frame);
BSPE_Error BSPE_PresentQueue_Dequeue(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame** out_frame);
BSPE_Error BSPE_PresentQueue_Peek(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame** out_frame);
bool       BSPE_PresentQueue_IsEmpty(BSPE_PresentQueueHandle handle);
bool       BSPE_PresentQueue_IsFull(BSPE_PresentQueueHandle handle);

/* --- Aliases Required by Step 5 Specification --- */
static inline BSPE_Error BSPE_PresentQueue_QueueFrame(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame* frame) {
    return BSPE_PresentQueue_Enqueue(handle, frame);
}
static inline BSPE_Error BSPE_PresentQueue_AcquireNextFrame(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame** out_frame) {
    return BSPE_PresentQueue_Dequeue(handle, out_frame);
}

/* --- Telemetry & Diagnostic APIs --- */
BSPE_Error BSPE_PresentQueue_GetStats(BSPE_PresentQueueHandle handle, BSPE_PresentQueueStats* out_stats);
bool       BSPE_PresentQueue_RunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_OS_BSPE_PRESENT_QUEUE_H
