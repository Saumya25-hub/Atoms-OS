#ifndef SIGNATURES_USB_REQUEST_QUEUE_H
#define SIGNATURES_USB_REQUEST_QUEUE_H

#include "../common/usb_common.h"
#include "../urb/usb_urb.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    urb_t* head;
    urb_t* tail;
    uint32_t count;
    atoms_spinlock_t lock;
} urb_queue_t;

typedef struct {
    urb_queue_t pending;
    urb_queue_t running;
    urb_queue_t completed;
    urb_queue_t cancelled;
    urb_queue_t timeout;
    urb_queue_t retry;
    urb_queue_t priority;
    atoms_spinlock_t lock;
} usb_request_queue_system_t;

// Queue Engine APIs
void usb_request_queue_init(void);

bool usb_request_queue_enqueue_pending(urb_t* urb);
bool usb_request_queue_enqueue_priority(urb_t* urb);
urb_t* usb_request_queue_pop_next(void);

bool usb_request_queue_move_running(urb_t* urb);
bool usb_request_queue_move_completed(urb_t* urb);
bool usb_request_queue_move_timeout(urb_t* urb);
bool usb_request_queue_move_retry(urb_t* urb);
bool usb_request_queue_cancel(urb_t* urb);

usb_request_queue_system_t* usb_get_request_queue_system(void);

#endif // SIGNATURES_USB_REQUEST_QUEUE_H
