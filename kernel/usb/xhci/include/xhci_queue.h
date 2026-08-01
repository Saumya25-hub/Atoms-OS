#ifndef SIGNATURES_XHCI_QUEUE_H
#define SIGNATURES_XHCI_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/sync/spinlock.h"

typedef enum {
    XHCI_REQ_STATE_IDLE = 0,
    XHCI_REQ_STATE_PENDING,
    XHCI_REQ_STATE_RUNNING,
    XHCI_REQ_STATE_COMPLETED,
    XHCI_REQ_STATE_TIMED_OUT,
    XHCI_REQ_STATE_CANCELLED,
    XHCI_REQ_STATE_ERROR
} xhci_req_state_t;

typedef struct xhci_bulk_request {
    uint32_t request_id;
    uint8_t  slot_id;
    uint8_t  ep_num;       // Endpoint number (1..15)
    bool     dir_in;       // true = IN, false = OUT
    
    void*    virt_buffer;  // Virtual address of memory buffer
    uint64_t phys_buffer;  // Physical address for DMA
    uint32_t transfer_len; // Requested byte length
    uint32_t actual_len;   // Transferred byte length
    
    uint32_t timeout_ms;   // Timeout in milliseconds (0 = no timeout)
    uint64_t submit_time_ms;
    
    xhci_req_state_t state;
    uint32_t completion_code;
    
    // Callback function on transfer completion
    void (*completion_cb)(struct xhci_bulk_request* req);
    void* user_data;
    
    // Internal Queue Links
    struct xhci_bulk_request* prev;
    struct xhci_bulk_request* next;
} xhci_bulk_request_t;

typedef struct {
    xhci_bulk_request_t* head;
    xhci_bulk_request_t* tail;
    uint32_t count;
    atoms_spinlock_t lock;
} xhci_request_queue_t;

typedef struct {
    xhci_request_queue_t pending_queue;
    xhci_request_queue_t running_queue;
    xhci_request_queue_t completed_queue;
    xhci_request_queue_t timeout_queue;
    uint32_t total_submitted;
    uint32_t total_completed;
    uint32_t total_errors;
} xhci_queue_manager_t;

// Queue Manager APIs
void xhci_queue_init(xhci_request_queue_t* q);
void xhci_queue_push_tail(xhci_request_queue_t* q, xhci_bulk_request_t* req);
xhci_bulk_request_t* xhci_queue_pop_head(xhci_request_queue_t* q);
bool xhci_queue_remove(xhci_request_queue_t* q, xhci_bulk_request_t* req);
void xhci_queue_manager_init(void);
xhci_queue_manager_t* xhci_get_queue_manager(void);

#endif // SIGNATURES_XHCI_QUEUE_H
