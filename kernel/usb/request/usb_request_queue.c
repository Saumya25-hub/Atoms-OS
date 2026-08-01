#include "usb_request_queue.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_request_queue_system_t g_queue_sys;

static void queue_init(urb_queue_t* q) {
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    atoms_spinlock_init(&q->lock, 1);
}

static void queue_push_tail(urb_queue_t* q, urb_t* urb) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    urb->next = NULL;
    urb->prev = q->tail;
    if (q->tail) q->tail->next = urb;
    else q->head = urb;
    q->tail = urb;
    q->count++;
    atoms_spin_unlock_irqrestore(&q->lock, state);
}

static urb_t* queue_pop_head(urb_queue_t* q) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    urb_t* urb = q->head;
    if (urb) {
        q->head = urb->next;
        if (q->head) q->head->prev = NULL;
        else q->tail = NULL;
        urb->next = NULL;
        urb->prev = NULL;
        q->count--;
    }
    atoms_spin_unlock_irqrestore(&q->lock, state);
    return urb;
}

static bool queue_remove(urb_queue_t* q, urb_t* urb) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&q->lock);
    bool found = false;
    urb_t* curr = q->head;
    while (curr) {
        if (curr == urb) {
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

void usb_request_queue_init(void) {
    memset(&g_queue_sys, 0, sizeof(usb_request_queue_system_t));
    queue_init(&g_queue_sys.pending);
    queue_init(&g_queue_sys.running);
    queue_init(&g_queue_sys.completed);
    queue_init(&g_queue_sys.cancelled);
    queue_init(&g_queue_sys.timeout);
    queue_init(&g_queue_sys.retry);
    queue_init(&g_queue_sys.priority);
    atoms_spinlock_init(&g_queue_sys.lock, 1);
    display_print("[USB QUEUE] 7-Tier Thread-Safe Request Queue Engine Initialized.\n");
}

bool usb_request_queue_enqueue_pending(urb_t* urb) {
    if (!urb) return false;
    urb->status = USB_URB_STATUS_PENDING;
    queue_push_tail(&g_queue_sys.pending, urb);
    return true;
}

bool usb_request_queue_enqueue_priority(urb_t* urb) {
    if (!urb) return false;
    urb->status = USB_URB_STATUS_PENDING;
    queue_push_tail(&g_queue_sys.priority, urb);
    return true;
}

urb_t* usb_request_queue_pop_next(void) {
    // Priority queue first
    urb_t* urb = queue_pop_head(&g_queue_sys.priority);
    if (!urb) {
        urb = queue_pop_head(&g_queue_sys.pending);
    }
    return urb;
}

bool usb_request_queue_move_running(urb_t* urb) {
    if (!urb) return false;
    urb->status = USB_URB_STATUS_IN_PROGRESS;
    queue_push_tail(&g_queue_sys.running, urb);
    return true;
}

bool usb_request_queue_move_completed(urb_t* urb) {
    if (!urb) return false;
    queue_remove(&g_queue_sys.running, urb);
    queue_remove(&g_queue_sys.pending, urb);
    urb->status = USB_URB_STATUS_COMPLETED;
    queue_push_tail(&g_queue_sys.completed, urb);
    return true;
}

bool usb_request_queue_move_timeout(urb_t* urb) {
    if (!urb) return false;
    queue_remove(&g_queue_sys.running, urb);
    urb->status = USB_URB_STATUS_TIMED_OUT;
    queue_push_tail(&g_queue_sys.timeout, urb);
    return true;
}

bool usb_request_queue_move_retry(urb_t* urb) {
    if (!urb) return false;
    queue_remove(&g_queue_sys.running, urb);
    queue_remove(&g_queue_sys.timeout, urb);
    urb->retry_count++;
    queue_push_tail(&g_queue_sys.retry, urb);
    return true;
}

bool usb_request_queue_cancel(urb_t* urb) {
    if (!urb) return false;
    if (queue_remove(&g_queue_sys.pending, urb) || queue_remove(&g_queue_sys.running, urb) || queue_remove(&g_queue_sys.priority, urb)) {
        urb->status = USB_URB_STATUS_CANCELLED;
        queue_push_tail(&g_queue_sys.cancelled, urb);
        return true;
    }
    return false;
}

usb_request_queue_system_t* usb_get_request_queue_system(void) {
    return &g_queue_sys;
}
