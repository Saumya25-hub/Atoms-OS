#include "usb_scheduler.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

#define MAX_USB_REQUEST_POOL 128
static usb_transfer_request_t g_request_pool[MAX_USB_REQUEST_POOL];
static bool g_request_used[MAX_USB_REQUEST_POOL];
static atoms_spinlock_t g_pool_lock;
static uint32_t g_next_req_id = 1;
static usb_scheduler_t g_usb_scheduler;

static void list_init(usb_request_list_t* list) {
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    atoms_spinlock_init(&list->lock, 1);
}

static void list_push_tail(usb_request_list_t* list, usb_transfer_request_t* req) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&list->lock);
    req->next = NULL;
    req->prev = list->tail;
    if (list->tail) list->tail->next = req;
    else list->head = req;
    list->tail = req;
    list->count++;
    atoms_spin_unlock_irqrestore(&list->lock, state);
}

static usb_transfer_request_t* list_pop_head(usb_request_list_t* list) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&list->lock);
    usb_transfer_request_t* req = list->head;
    if (req) {
        list->head = req->next;
        if (list->head) list->head->prev = NULL;
        else list->tail = NULL;
        req->next = NULL;
        req->prev = NULL;
        list->count--;
    }
    atoms_spin_unlock_irqrestore(&list->lock, state);
    return req;
}

static bool list_remove(usb_request_list_t* list, usb_transfer_request_t* req) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&list->lock);
    bool found = false;
    usb_transfer_request_t* curr = list->head;
    while (curr) {
        if (curr == req) {
            if (curr->prev) curr->prev->next = curr->next;
            else list->head = curr->next;
            if (curr->next) curr->next->prev = curr->prev;
            else list->tail = curr->prev;
            curr->next = NULL;
            curr->prev = NULL;
            list->count--;
            found = true;
            break;
        }
        curr = curr->next;
    }
    atoms_spin_unlock_irqrestore(&list->lock, state);
    return found;
}

void usb_scheduler_init(void) {
    memset(g_request_pool, 0, sizeof(g_request_pool));
    memset(g_request_used, 0, sizeof(g_request_used));
    atoms_spinlock_init(&g_pool_lock, 1);
    
    memset(&g_usb_scheduler, 0, sizeof(usb_scheduler_t));
    list_init(&g_usb_scheduler.pending);
    list_init(&g_usb_scheduler.running);
    list_init(&g_usb_scheduler.completed);
    list_init(&g_usb_scheduler.timeout);
    list_init(&g_usb_scheduler.retry);
    atoms_spinlock_init(&g_usb_scheduler.lock, 1);
}

usb_transfer_request_t* usb_request_alloc(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pool_lock);
    for (uint32_t i = 0; i < MAX_USB_REQUEST_POOL; i++) {
        if (!g_request_used[i]) {
            g_request_used[i] = true;
            memset(&g_request_pool[i], 0, sizeof(usb_transfer_request_t));
            g_request_pool[i].req_id = g_next_req_id++;
            atoms_spin_unlock_irqrestore(&g_pool_lock, state);
            return &g_request_pool[i];
        }
    }
    atoms_spin_unlock_irqrestore(&g_pool_lock, state);
    return NULL;
}

void usb_request_free(usb_transfer_request_t* req) {
    if (!req) return;
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_pool_lock);
    uint64_t idx = ((uint64_t)req - (uint64_t)g_request_pool) / sizeof(usb_transfer_request_t);
    if (idx < MAX_USB_REQUEST_POOL) {
        g_request_used[idx] = false;
    }
    atoms_spin_unlock_irqrestore(&g_pool_lock, state);
}

bool usb_scheduler_submit(usb_transfer_request_t* req) {
    if (!req) return false;
    req->state = USB_SCHED_STATE_PENDING;
    req->submit_tick = timer_get_ticks();
    list_push_tail(&g_usb_scheduler.pending, req);
    return true;
}

bool usb_scheduler_cancel(usb_transfer_request_t* req) {
    if (!req) return false;
    if (list_remove(&g_usb_scheduler.pending, req) || list_remove(&g_usb_scheduler.running, req)) {
        req->state = USB_SCHED_STATE_CANCELLED;
        list_push_tail(&g_usb_scheduler.completed, req);
        if (req->completion_cb) req->completion_cb(req);
        return true;
    }
    return false;
}

void usb_scheduler_tick(void) {
    uint64_t now = timer_get_ticks();
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_usb_scheduler.running.lock);
    usb_transfer_request_t* curr = g_usb_scheduler.running.head;
    while (curr) {
        usb_transfer_request_t* next = curr->next;
        if (curr->timeout_ms > 0) {
            uint64_t elapsed = (curr->submit_tick == 0) ? (curr->timeout_ms + 1000) : (now - curr->submit_tick);
            if (elapsed > curr->timeout_ms) {
                curr->state = USB_SCHED_STATE_TIMED_OUT;
            }
        }
        curr = next;
    }
    atoms_spin_unlock_irqrestore(&g_usb_scheduler.running.lock, state);
}

usb_scheduler_t* usb_get_scheduler(void) {
    return &g_usb_scheduler;
}
