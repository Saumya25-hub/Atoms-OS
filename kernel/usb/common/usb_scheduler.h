#ifndef SIGNATURES_USB_SCHEDULER_H
#define SIGNATURES_USB_SCHEDULER_H

#include "usb_common.h"
#include "kernel/core/sync/spinlock.h"

typedef enum {
    USB_SCHED_STATE_IDLE = 0,
    USB_SCHED_STATE_PENDING,
    USB_SCHED_STATE_RUNNING,
    USB_SCHED_STATE_COMPLETED,
    USB_SCHED_STATE_TIMED_OUT,
    USB_SCHED_STATE_RETRY,
    USB_SCHED_STATE_CANCELLED,
    USB_SCHED_STATE_ERROR
} usb_sched_state_t;

typedef struct usb_transfer_request {
    uint32_t req_id;
    usb_controller_type_t controller_type;
    uint8_t  controller_id;
    uint8_t  dev_addr;
    uint8_t  ep_num;
    usb_direction_t dir;
    usb_transfer_type_t type;
    
    void*    virt_buf;
    uint64_t phys_buf;
    uint32_t req_len;
    uint32_t actual_len;
    
    uint32_t timeout_ms;
    uint64_t submit_tick;
    uint32_t retries;
    uint32_t max_retries;
    
    uint8_t  priority; // 0 = Lowest, 255 = Highest
    usb_sched_state_t state;
    uint32_t status_code;
    
    void (*completion_cb)(struct usb_transfer_request* req);
    void* user_data;
    
    struct usb_transfer_request* prev;
    struct usb_transfer_request* next;
} usb_transfer_request_t;

typedef struct {
    usb_transfer_request_t* head;
    usb_transfer_request_t* tail;
    uint32_t count;
    atoms_spinlock_t lock;
} usb_request_list_t;

typedef struct {
    usb_request_list_t pending;
    usb_request_list_t running;
    usb_request_list_t completed;
    usb_request_list_t timeout;
    usb_request_list_t retry;
    atoms_spinlock_t lock;
} usb_scheduler_t;

// Scheduler APIs
void usb_scheduler_init(void);
usb_transfer_request_t* usb_request_alloc(void);
void usb_request_free(usb_transfer_request_t* req);
bool usb_scheduler_submit(usb_transfer_request_t* req);
bool usb_scheduler_cancel(usb_transfer_request_t* req);
void usb_scheduler_tick(void);
usb_scheduler_t* usb_get_scheduler(void);

#endif // SIGNATURES_USB_SCHEDULER_H
