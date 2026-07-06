/**
 * @file present_queue.c
 * @brief BSPE Present Queue Production Implementation
 * @status Step 5 Production Implementation
 * 
 * @section PURPOSE
 * Implements a lock-free, Single Producer Single Consumer (SPSC) fixed-size ring buffer
 * using atomic head and tail indices without heap allocation, mutexes, or spinlocks.
 */

#include "present_queue.h"
#include <stddef.h>

#define BSPE_PQ_MAX_INSTANCES 4
#define BSPE_PQ_RING_CAPACITY 16 /* Must be power of 2 */
#define BSPE_PQ_RING_MASK     (BSPE_PQ_RING_CAPACITY - 1)

typedef struct BSPE_PresentQueue_T {
    const BOGE_StagingFrame* ring_buffer[BSPE_PQ_RING_CAPACITY];
    volatile uint32_t head; /* Modified strictly by Producer (Enqueue) */
    volatile uint32_t tail; /* Modified strictly by Consumer (Dequeue) */
    
    /* Telemetry counters */
    uint32_t total_enqueued;
    uint32_t total_dequeued;
    uint32_t dropped_frames;
    
    bool is_allocated;
} BSPE_PresentQueueInstance;

/* Static pool in kernel BSS segment (No heap allocation) */
static BSPE_PresentQueueInstance g_pq_pool[BSPE_PQ_MAX_INSTANCES];

/* --- Lifecycle APIs --- */

BSPE_Error BSPE_PresentQueue_Create(const BSPE_PresentQueueConfig* config, BSPE_PresentQueueHandle* out_handle) {
    if (!out_handle) return BSPE_ERR_NULL_POINTER;
    (void)config; /* Capacity fixed to BSPE_PQ_RING_CAPACITY in zero-heap model */
    
    for (uint32_t i = 0; i < BSPE_PQ_MAX_INSTANCES; i++) {
        if (!g_pq_pool[i].is_allocated) {
            g_pq_pool[i].is_allocated = true;
            g_pq_pool[i].head = 0;
            g_pq_pool[i].tail = 0;
            g_pq_pool[i].total_enqueued = 0;
            g_pq_pool[i].total_dequeued = 0;
            g_pq_pool[i].dropped_frames = 0;
            for (uint32_t j = 0; j < BSPE_PQ_RING_CAPACITY; j++) {
                g_pq_pool[i].ring_buffer[j] = NULL;
            }
            *out_handle = (BSPE_PresentQueueHandle)&g_pq_pool[i];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_PresentQueue_Destroy(BSPE_PresentQueueHandle handle) {
    if (!handle) return;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    pq->is_allocated = false;
    pq->head = 0;
    pq->tail = 0;
}

void BSPE_PresentQueue_Reset(BSPE_PresentQueueHandle handle) {
    if (!handle) return;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    
    /* Use atomic release store to reset indices */
    __atomic_store_n(&pq->tail, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&pq->head, 0, __ATOMIC_RELEASE);
    pq->total_enqueued = 0;
    pq->total_dequeued = 0;
    pq->dropped_frames = 0;
}

/* --- Core SPSC Lock-Free Operations --- */

bool BSPE_PresentQueue_IsEmpty(BSPE_PresentQueueHandle handle) {
    if (!handle) return true;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_ACQUIRE);
    return (h == t);
}

bool BSPE_PresentQueue_IsFull(BSPE_PresentQueueHandle handle) {
    if (!handle) return true;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_ACQUIRE);
    return ((h - t) >= BSPE_PQ_RING_CAPACITY);
}

BSPE_Error BSPE_PresentQueue_Enqueue(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame* frame) {
    if (!handle || !frame) return BSPE_ERR_NULL_POINTER;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_ACQUIRE);
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_RELAXED);
    
    /* Check if queue is saturated */
    if ((h - t) >= BSPE_PQ_RING_CAPACITY) {
        pq->dropped_frames++;
        return BSPE_ERR_QUEUE_FULL;
    }
    
    /* Write frame pointer into ring slot */
    pq->ring_buffer[h & BSPE_PQ_RING_MASK] = frame;
    
    /* Publish new head index with release barrier */
    __atomic_store_n(&pq->head, h + 1, __ATOMIC_RELEASE);
    pq->total_enqueued++;
    return BSPE_OK;
}

BSPE_Error BSPE_PresentQueue_Dequeue(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame** out_frame) {
    if (!handle || !out_frame) return BSPE_ERR_NULL_POINTER;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_RELAXED);
    
    /* Check if queue is empty */
    if (t == h) {
        *out_frame = NULL;
        return BSPE_ERR_QUEUE_EMPTY;
    }
    
    /* Read frame pointer from ring slot */
    *out_frame = pq->ring_buffer[t & BSPE_PQ_RING_MASK];
    
    /* Publish new tail index with release barrier */
    __atomic_store_n(&pq->tail, t + 1, __ATOMIC_RELEASE);
    pq->total_dequeued++;
    return BSPE_OK;
}

BSPE_Error BSPE_PresentQueue_Peek(BSPE_PresentQueueHandle handle, const BOGE_StagingFrame** out_frame) {
    if (!handle || !out_frame) return BSPE_ERR_NULL_POINTER;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_RELAXED);
    
    if (t == h) {
        *out_frame = NULL;
        return BSPE_ERR_QUEUE_EMPTY;
    }
    
    *out_frame = pq->ring_buffer[t & BSPE_PQ_RING_MASK];
    return BSPE_OK;
}

BSPE_Error BSPE_PresentQueue_GetStats(BSPE_PresentQueueHandle handle, BSPE_PresentQueueStats* out_stats) {
    if (!handle || !out_stats) return BSPE_ERR_NULL_POINTER;
    BSPE_PresentQueueInstance* pq = (BSPE_PresentQueueInstance*)handle;
    
    uint32_t h = __atomic_load_n(&pq->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&pq->tail, __ATOMIC_ACQUIRE);
    
    out_stats->total_enqueued = pq->total_enqueued;
    out_stats->total_dequeued = pq->total_dequeued;
    out_stats->dropped_frames = pq->dropped_frames;
    out_stats->current_depth = (h >= t) ? (h - t) : 0;
    return BSPE_OK;
}

/* --- Self-Test Verification Suite --- */

bool BSPE_PresentQueue_RunSelfTest(void) {
    BSPE_PresentQueueHandle q = NULL;
    BSPE_PresentQueueConfig cfg = { .capacity = 16 };
    
    if (BSPE_PresentQueue_Create(&cfg, &q) != BSPE_OK || !q) return false;
    
    /* 1. Underflow Test */
    const BOGE_StagingFrame* f_out = NULL;
    if (BSPE_PresentQueue_Dequeue(q, &f_out) != BSPE_ERR_QUEUE_EMPTY) return false;
    if (BSPE_PresentQueue_Peek(q, &f_out) != BSPE_ERR_QUEUE_EMPTY) return false;
    if (!BSPE_PresentQueue_IsEmpty(q)) return false;
    
    /* 2. Overflow Test (Capacity = 16) */
    BOGE_StagingFrame dummy_frames[17];
    for (int i = 0; i < 16; i++) {
        if (BSPE_PresentQueue_Enqueue(q, &dummy_frames[i]) != BSPE_OK) return false;
    }
    if (!BSPE_PresentQueue_IsFull(q)) return false;
    if (BSPE_PresentQueue_Enqueue(q, &dummy_frames[16]) != BSPE_ERR_QUEUE_FULL) return false;
    
    /* 3. Wrap-Around Test */
    for (int i = 0; i < 16; i++) {
        if (BSPE_PresentQueue_Dequeue(q, &f_out) != BSPE_OK || f_out != &dummy_frames[i]) return false;
    }
    if (!BSPE_PresentQueue_IsEmpty(q)) return false;
    
    /* Enqueue and dequeue 100 times across wrap-around boundaries */
    for (int iter = 0; iter < 100; iter++) {
        if (BSPE_PresentQueue_Enqueue(q, &dummy_frames[iter % 16]) != BSPE_OK) return false;
        if (BSPE_PresentQueue_Dequeue(q, &f_out) != BSPE_OK || f_out != &dummy_frames[iter % 16]) return false;
    }
    
    /* 4. Stress Interleaving Test */
    for (int i = 0; i < 8; i++) BSPE_PresentQueue_Enqueue(q, &dummy_frames[i]);
    for (int i = 0; i < 4; i++) BSPE_PresentQueue_Dequeue(q, &f_out);
    for (int i = 0; i < 8; i++) BSPE_PresentQueue_Enqueue(q, &dummy_frames[i]);
    
    BSPE_PresentQueueStats stats;
    BSPE_PresentQueue_GetStats(q, &stats);
    if (stats.current_depth != 12) return false;
    
    BSPE_PresentQueue_Destroy(q);
    return true;
}

#ifdef BSPE_TEST_HARNESS
#include <stdio.h>
int main(void) {
    printf("[BSPE Test] Running Present Queue Self-Test Suite...\n");
    if (BSPE_PresentQueue_RunSelfTest()) {
        printf("[BSPE Test] ALL TESTS PASSED: Underflow, Overflow, Wrap-around, Stress!\n");
        return 0;
    } else {
        printf("[BSPE Test] SELF-TEST FAILED!\n");
        return 1;
    }
}
#endif
