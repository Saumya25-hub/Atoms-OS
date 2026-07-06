/**
 * @file swapchain.c
 * @brief BSPE Swapchain Production Implementation
 * @status Step 7 Production Implementation
 * 
 * @section PURPOSE
 * Implements a double and triple buffering state machine without heap allocation or VRAM operations.
 * Enforces strict transition validation: FREE -> ACQUIRED -> QUEUED -> DISPLAYING -> FREE.
 */

#include "swapchain.h"
#include <stddef.h>

#define BSPE_SC_MAX_BUFFERS   3
#define BSPE_SC_MAX_INSTANCES 4

typedef struct BSPE_Swapchain_T {
    BSPE_SwapchainBuffer buffers[BSPE_SC_MAX_BUFFERS];
    uint32_t buffer_count; /* 2 or 3 */
    uint32_t width;
    uint32_t height;
    uint32_t vsync_interval;
    
    /* Telemetry & Leak Tracking */
    uint32_t total_acquires;
    uint32_t total_queues;
    uint32_t total_displays;
    uint32_t total_releases;
    
    bool is_allocated;
} BSPE_SwapchainInstance;

/* Static pool in kernel BSS segment (No heap allocation) */
static BSPE_SwapchainInstance g_sc_pool[BSPE_SC_MAX_INSTANCES];

/* --- Lifecycle Implementations --- */

BSPE_Error BSPE_Swapchain_Create(const BSPE_SwapchainConfig* config, BSPE_SwapchainHandle* out_handle) {
    if (!out_handle) return BSPE_ERR_NULL_POINTER;
    if (!config || (config->buffer_count < 2 || config->buffer_count > BSPE_SC_MAX_BUFFERS)) {
        return BSPE_ERR_INVALID_STATE;
    }
    
    for (uint32_t i = 0; i < BSPE_SC_MAX_INSTANCES; i++) {
        if (!g_sc_pool[i].is_allocated) {
            g_sc_pool[i].is_allocated = true;
            g_sc_pool[i].buffer_count = config->buffer_count;
            g_sc_pool[i].width        = config->width ? config->width : 1024;
            g_sc_pool[i].height       = config->height ? config->height : 768;
            g_sc_pool[i].vsync_interval = config->vsync_interval;
            g_sc_pool[i].total_acquires = 0;
            g_sc_pool[i].total_queues   = 0;
            g_sc_pool[i].total_displays = 0;
            g_sc_pool[i].total_releases = 0;
            
            /* Initialize buffer pool */
            for (uint32_t b = 0; b < g_sc_pool[i].buffer_count; b++) {
                g_sc_pool[i].buffers[b].buffer_id = b;
                g_sc_pool[i].buffers[b].width     = g_sc_pool[i].width;
                g_sc_pool[i].buffers[b].height    = g_sc_pool[i].height;
                g_sc_pool[i].buffers[b].pitch     = g_sc_pool[i].width * 4;
                g_sc_pool[i].buffers[b].virtual_address  = NULL; /* No VRAM operations in Step 7 */
                g_sc_pool[i].buffers[b].physical_address = 0;
                g_sc_pool[i].buffers[b].is_vram_page     = true;
                
                /* Buffer 0 starts out displaying front; others start free */
                if (b == 0) {
                    g_sc_pool[i].buffers[b].state = BSPE_BUFFER_STATE_DISPLAYING_FRONT;
                } else {
                    g_sc_pool[i].buffers[b].state = BSPE_BUFFER_STATE_FREE;
                }
            }
            
            *out_handle = (BSPE_SwapchainHandle)&g_sc_pool[i];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_Swapchain_Destroy(BSPE_SwapchainHandle handle) {
    if (!handle) return;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    sc->is_allocated = false;
}

void BSPE_Swapchain_Reset(BSPE_SwapchainHandle handle) {
    if (!handle) return;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (b == 0) {
            sc->buffers[b].state = BSPE_BUFFER_STATE_DISPLAYING_FRONT;
        } else {
            sc->buffers[b].state = BSPE_BUFFER_STATE_FREE;
        }
    }
}

BSPE_Error BSPE_Swapchain_Resize(BSPE_SwapchainHandle handle, uint32_t new_width, uint32_t new_height) {
    if (!handle) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    sc->width  = new_width;
    sc->height = new_height;
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        sc->buffers[b].width  = new_width;
        sc->buffers[b].height = new_height;
        sc->buffers[b].pitch  = new_width * 4;
    }
    return BSPE_OK;
}

/* --- Core Buffer State Machine Implementations --- */

BSPE_Error BSPE_Swapchain_AcquireNextBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    if (!handle || !out_buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    /* Find first FREE buffer in pool */
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (sc->buffers[b].state == BSPE_BUFFER_STATE_FREE) {
            sc->buffers[b].state = BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE;
            sc->total_acquires++;
            *out_buffer = &sc->buffers[b];
            return BSPE_OK;
        }
    }
    
    /* No free buffer available (All acquired, queued, or displaying) */
    *out_buffer = NULL;
    return BSPE_ERR_QUEUE_FULL;
}

BSPE_Error BSPE_Swapchain_Present(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer* buffer) {
    if (!handle || !buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    /* State Transition Validation: Must be strictly ACQUIRED_BY_BOGE */
    if (buffer->state != BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE) {
        return BSPE_ERR_INVALID_STATE;
    }
    
    buffer->state = BSPE_BUFFER_STATE_QUEUED_IN_BSPE;
    sc->total_queues++;
    return BSPE_OK;
}

BSPE_Error BSPE_Swapchain_AcquireDisplayBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    if (!handle || !out_buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    /* Find first QUEUED buffer in pool (FIFO order by ID/index) */
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (sc->buffers[b].state == BSPE_BUFFER_STATE_QUEUED_IN_BSPE) {
            sc->buffers[b].state = BSPE_BUFFER_STATE_DISPLAYING_FRONT;
            sc->total_displays++;
            *out_buffer = &sc->buffers[b];
            return BSPE_OK;
        }
    }
    
    *out_buffer = NULL;
    return BSPE_ERR_QUEUE_EMPTY;
}

BSPE_Error BSPE_Swapchain_ReleaseDisplayBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer* buffer) {
    if (!handle || !buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    /* State Transition Validation: Must be strictly DISPLAYING_FRONT */
    if (buffer->state != BSPE_BUFFER_STATE_DISPLAYING_FRONT) {
        return BSPE_ERR_INVALID_STATE;
    }
    
    buffer->state = BSPE_BUFFER_STATE_FREE;
    sc->total_releases++;
    return BSPE_OK;
}

BSPE_Error BSPE_Swapchain_GetFrontBuffer(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    if (!handle || !out_buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (sc->buffers[b].state == BSPE_BUFFER_STATE_DISPLAYING_FRONT) {
            *out_buffer = &sc->buffers[b];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_INVALID_STATE;
}

BSPE_Error BSPE_Swapchain_GetCurrentBack(BSPE_SwapchainHandle handle, BSPE_SwapchainBuffer** out_buffer) {
    if (!handle || !out_buffer) return BSPE_ERR_NULL_POINTER;
    BSPE_SwapchainInstance* sc = (BSPE_SwapchainInstance*)handle;
    
    /* Look for active acquired buffer first */
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (sc->buffers[b].state == BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE) {
            *out_buffer = &sc->buffers[b];
            return BSPE_OK;
        }
    }
    
    /* If none acquired, return next free buffer */
    for (uint32_t b = 0; b < sc->buffer_count; b++) {
        if (sc->buffers[b].state == BSPE_BUFFER_STATE_FREE) {
            *out_buffer = &sc->buffers[b];
            return BSPE_OK;
        }
    }
    
    return BSPE_ERR_INVALID_STATE;
}

/* --- Self-Test Verification Suite --- */

bool BSPE_Swapchain_RunSelfTest(void) {
    BSPE_SwapchainHandle sc = NULL;
    BSPE_SwapchainConfig cfg = { .width = 1920, .height = 1080, .buffer_count = 2 };
    
    /* 1. Double-Buffer Rotation Test (N=2) */
    if (BSPE_Swapchain_Create(&cfg, &sc) != BSPE_OK || !sc) return false;
    
    BSPE_SwapchainBuffer* buf_a = NULL;
    BSPE_SwapchainBuffer* buf_b = NULL;
    BSPE_SwapchainBuffer* front = NULL;
    
    /* Buffer 0 is DISPLAYING, Buffer 1 is FREE */
    if (BSPE_Swapchain_AcquireBuffer(sc, &buf_a) != BSPE_OK || buf_a->buffer_id != 1) return false;
    if (buf_a->state != BSPE_BUFFER_STATE_ACQUIRED_BY_BOGE) return false;
    
    /* Trying to acquire again in double-buffer must fail (queue full) */
    if (BSPE_Swapchain_AcquireBuffer(sc, &buf_b) != BSPE_ERR_QUEUE_FULL) return false;
    
    /* Queue buffer 1 */
    if (BSPE_Swapchain_QueueBuffer(sc, buf_a) != BSPE_OK) return false;
    if (buf_a->state != BSPE_BUFFER_STATE_QUEUED_IN_BSPE) return false;
    
    /* Display engine acquires buffer 1 to display */
    if (BSPE_Swapchain_AcquireDisplayBuffer(sc, &buf_b) != BSPE_OK || buf_b != buf_a) return false;
    if (buf_b->state != BSPE_BUFFER_STATE_DISPLAYING_FRONT) return false;
    
    /* Get old front (Buffer 0) and release it */
    BSPE_SwapchainBuffer* old_front = ((BSPE_SwapchainInstance*)sc)->buffers; /* Buffer 0 */
    if (BSPE_Swapchain_ReleaseDisplayBuffer(sc, old_front) != BSPE_OK) return false;
    if (old_front->state != BSPE_BUFFER_STATE_FREE) return false;
    
    /* Now Buffer 0 is FREE, we can acquire it! */
    if (BSPE_Swapchain_AcquireBuffer(sc, &buf_a) != BSPE_OK || buf_a->buffer_id != 0) return false;
    BSPE_Swapchain_Destroy(sc);
    
    /* 2. Triple-Buffer Rotation Test (N=3) */
    cfg.buffer_count = 3;
    if (BSPE_Swapchain_Create(&cfg, &sc) != BSPE_OK || !sc) return false;
    
    /* Buffer 0 is DISPLAYING, Buffer 1 is FREE, Buffer 2 is FREE */
    if (BSPE_Swapchain_AcquireBuffer(sc, &buf_a) != BSPE_OK || buf_a->buffer_id != 1) return false;
    if (BSPE_Swapchain_QueueBuffer(sc, buf_a) != BSPE_OK) return false;
    
    /* In triple buffering, while Buffer 1 is queued, we can acquire Buffer 2! */
    if (BSPE_Swapchain_AcquireBuffer(sc, &buf_b) != BSPE_OK || buf_b->buffer_id != 2) return false;
    if (BSPE_Swapchain_QueueBuffer(sc, buf_b) != BSPE_OK) return false;
    
    /* Both 1 and 2 are queued. Acquire display buffer gets 1 (FIFO order) */
    if (BSPE_Swapchain_AcquireDisplayBuffer(sc, &front) != BSPE_OK || front->buffer_id != 1) return false;
    BSPE_Swapchain_Destroy(sc);
    
    /* 3. Invalid Transition Test */
    if (BSPE_Swapchain_Create(&cfg, &sc) != BSPE_OK || !sc) return false;
    BSPE_SwapchainBuffer* free_buf = &((BSPE_SwapchainInstance*)sc)->buffers[1];
    /* Try to queue a FREE buffer without acquiring -> Must fail */
    if (BSPE_Swapchain_QueueBuffer(sc, free_buf) != BSPE_ERR_INVALID_STATE) return false;
    /* Try to release a FREE buffer -> Must fail */
    if (BSPE_Swapchain_ReleaseDisplayBuffer(sc, free_buf) != BSPE_ERR_INVALID_STATE) return false;
    BSPE_Swapchain_Destroy(sc);
    
    /* 4. Stress & Buffer Leak Detection Test (1000 cycles on Triple Buffering) */
    if (BSPE_Swapchain_Create(&cfg, &sc) != BSPE_OK || !sc) return false;
    for (int iter = 0; iter < 1000; iter++) {
        BSPE_SwapchainBuffer* acq = NULL;
        BSPE_SwapchainBuffer* disp = NULL;
        BSPE_SwapchainBuffer* old_f = NULL;
        
        BSPE_Swapchain_GetCurrentFront(sc, &old_f);
        if (BSPE_Swapchain_AcquireBuffer(sc, &acq) != BSPE_OK) return false;
        if (BSPE_Swapchain_QueueBuffer(sc, acq) != BSPE_OK) return false;
        if (BSPE_Swapchain_AcquireDisplayBuffer(sc, &disp) != BSPE_OK) return false;
        if (BSPE_Swapchain_ReleaseDisplayBuffer(sc, old_f) != BSPE_OK) return false;
    }
    
    /* Check leak invariants: exactly 1 displaying, 2 free */
    BSPE_SwapchainInstance* inst = (BSPE_SwapchainInstance*)sc;
    int disp_cnt = 0, free_cnt = 0;
    for (int b = 0; b < 3; b++) {
        if (inst->buffers[b].state == BSPE_BUFFER_STATE_DISPLAYING_FRONT) disp_cnt++;
        if (inst->buffers[b].state == BSPE_BUFFER_STATE_FREE) free_cnt++;
    }
    if (disp_cnt != 1 || free_cnt != 2) return false;
    if (inst->total_acquires != 1000 || inst->total_releases != 1000) return false;
    
    BSPE_Swapchain_Destroy(sc);
    return true;
}

#ifdef BSPE_TEST_HARNESS
#include <stdio.h>
int main(void) {
    printf("[BSPE Test] Running Swapchain Self-Test Suite...\n");
    if (BSPE_Swapchain_RunSelfTest()) {
        printf("[BSPE Test] ALL TESTS PASSED: Double-Buffer, Triple-Buffer, Invalid Transitions, Leak Detection, Stress!\n");
        return 0;
    } else {
        printf("[BSPE Test] SELF-TEST FAILED!\n");
        return 1;
    }
}
#endif
