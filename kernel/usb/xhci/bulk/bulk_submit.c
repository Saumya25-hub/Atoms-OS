#include "../include/xhci_bulk.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

#define MAX_BULK_REQUESTS 128
static xhci_bulk_request_t g_request_pool[MAX_BULK_REQUESTS];
static bool g_request_used[MAX_BULK_REQUESTS];
static atoms_spinlock_t g_pool_lock;
static uint32_t g_next_request_id = 1;

extern void xhci_scheduler_dispatch_pending(void);

void xhci_bte_pool_init(void) {
    memset(g_request_pool, 0, sizeof(g_request_pool));
    memset(g_request_used, 0, sizeof(g_request_used));
    atoms_spinlock_init(&g_pool_lock, 1);
}

xhci_bulk_request_t* xhci_bulk_request_alloc(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pool_lock);
    for (uint32_t i = 0; i < MAX_BULK_REQUESTS; i++) {
        if (!g_request_used[i]) {
            g_request_used[i] = true;
            memset(&g_request_pool[i], 0, sizeof(xhci_bulk_request_t));
            g_request_pool[i].request_id = g_next_request_id++;
            atoms_spin_unlock_irqrestore(&g_pool_lock, state);
            return &g_request_pool[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_pool_lock, state);
    return NULL;
}

void xhci_bulk_request_free(xhci_bulk_request_t* req) {
    if (!req) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pool_lock);
    uint64_t index = ((uint64_t)req - (uint64_t)g_request_pool) / sizeof(xhci_bulk_request_t);
    if (index < MAX_BULK_REQUESTS) {
        g_request_used[index] = false;
    }
    atoms_spin_unlock_irqrestore(&g_pool_lock, state);
}

bool xhci_bulk_submit(xhci_bulk_request_t* req) {
    if (!req) return false;
    
    req->state = XHCI_REQ_STATE_PENDING;
    req->submit_time_ms = timer_get_ticks();
    
    xhci_queue_manager_t* mgr = xhci_get_queue_manager();
    xhci_queue_push_tail(&mgr->pending_queue, req);
    
    // Dispatch to transfer ring
    xhci_scheduler_dispatch_pending();
    return true;
}

static void sync_bulk_completion_cb(xhci_bulk_request_t* req) {
    if (req && req->user_data) {
        volatile bool* done_ptr = (volatile bool*)req->user_data;
        *done_ptr = true;
    }
}

bool xhci_bulk_transfer_sync(uint8_t slot_id, uint8_t ep_num, bool dir_in, void* buffer, uint32_t len, uint32_t timeout_ms, uint32_t* actual_len) {
    volatile bool done = false;
    
    bool submitted = false;
    if (dir_in) {
        submitted = xhci_bulk_in(slot_id, ep_num, buffer, len, timeout_ms, sync_bulk_completion_cb, (void*)&done);
    } else {
        submitted = xhci_bulk_out(slot_id, ep_num, buffer, len, timeout_ms, sync_bulk_completion_cb, (void*)&done);
    }
    
    if (!submitted) return false;
    
    uint64_t start_ms = timer_get_ticks();
    while (!done) {
        extern void xhci_poll(void);
        xhci_poll();
        
        uint64_t elapsed = timer_get_ticks() - start_ms;
        if (timeout_ms > 0 && elapsed > timeout_ms) {
            display_print("[XHCI BTE] Sync Bulk Transfer Timeout!\n");
            return false;
        }
        __asm__ volatile ("pause");
    }
    
    return true;
}
