#include "../include/xhci_queue.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/lib/include/string.h"

static xhci_queue_manager_t g_queue_mgr;

void xhci_queue_init(xhci_request_queue_t* q) {
    if (!q) return;
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    atoms_spinlock_init(&q->lock, 1);
}

void xhci_queue_push_tail(xhci_request_queue_t* q, xhci_bulk_request_t* req) {
    if (!q || !req) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    
    req->next = NULL;
    req->prev = q->tail;
    
    if (q->tail) {
        q->tail->next = req;
    } else {
        q->head = req;
    }
    q->tail = req;
    q->count++;
    
    atoms_spin_unlock_irqrestore(&q->lock, state);
}

xhci_bulk_request_t* xhci_queue_pop_head(xhci_request_queue_t* q) {
    if (!q) return NULL;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    
    xhci_bulk_request_t* req = q->head;
    if (req) {
        q->head = req->next;
        if (q->head) {
            q->head->prev = NULL;
        } else {
            q->tail = NULL;
        }
        req->next = NULL;
        req->prev = NULL;
        q->count--;
    }
    
    atoms_spin_unlock_irqrestore(&q->lock, state);
    return req;
}

bool xhci_queue_remove(xhci_request_queue_t* q, xhci_bulk_request_t* req) {
    if (!q || !req) return false;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    
    bool found = false;
    xhci_bulk_request_t* curr = q->head;
    while (curr) {
        if (curr == req) {
            if (curr->prev) curr->prev->next = curr->next;
            else q->head = curr->next;
            
            if (curr->next) curr->next->prev = curr->prev;
            else q->tail = curr->prev;
            
            curr->next = NULL;
            curr->prev = NULL;
            q->count--;
            found = true;
            break;
        }
        curr = curr->next;
    }
    
    atoms_spin_unlock_irqrestore(&q->lock, state);
    return found;
}

void xhci_queue_manager_init(void) {
    memset(&g_queue_mgr, 0, sizeof(xhci_queue_manager_t));
    xhci_queue_init(&g_queue_mgr.pending_queue);
    xhci_queue_init(&g_queue_mgr.running_queue);
    xhci_queue_init(&g_queue_mgr.completed_queue);
    xhci_queue_init(&g_queue_mgr.timeout_queue);
}

xhci_queue_manager_t* xhci_get_queue_manager(void) {
    return &g_queue_mgr;
}
