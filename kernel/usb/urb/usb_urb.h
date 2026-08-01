#ifndef SIGNATURES_USB_URB_H
#define SIGNATURES_USB_URB_H

#include "../common/usb_common.h"
#include "../core/usb_core.h"
#include "kernel/core/sync/spinlock.h"

#define MAX_URB_POOL_SIZE 512

// URB Status Codes
typedef enum {
    USB_URB_STATUS_PENDING = 0,
    USB_URB_STATUS_IN_PROGRESS,
    USB_URB_STATUS_COMPLETED,
    USB_URB_STATUS_CANCELLED,
    USB_URB_STATUS_TIMED_OUT,
    USB_URB_STATUS_STALLED,
    USB_URB_STATUS_CRC_ERROR,
    USB_URB_STATUS_BITSTUFF_ERROR,
    USB_URB_STATUS_BABBLE_ERROR,
    USB_URB_STATUS_DATA_BUFFER_ERROR,
    USB_URB_STATUS_HARDWARE_ERROR,
    USB_URB_STATUS_INVALID_PARAM
} usb_urb_status_t;

// URB Transfer Flags
#define URB_FLAG_SHORT_NOT_OK   (1 << 0)
#define URB_FLAG_ISO_ASAP       (1 << 1)
#define URB_FLAG_NO_TRANSFER_DMA (1 << 2)
#define URB_FLAG_NO_FSBR        (1 << 3)
#define URB_FLAG_ZERO_PACKET    (1 << 4)

struct urb;
typedef void (*usb_complete_t)(struct urb* urb);

// USB Setup Packet Struct (for Control Transfers)
typedef struct __attribute__((packed)) {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// Universal Request Block (URB)
typedef struct urb {
    uint32_t urb_id;
    usb_device_t* dev;
    uint32_t pipe;
    usb_transfer_type_t type;
    usb_direction_t dir;
    uint32_t transfer_flags;
    
    usb_setup_packet_t setup_packet;
    void* transfer_buffer;
    uint64_t transfer_dma;
    uint32_t transfer_buffer_length;
    uint32_t actual_length;
    
    uint32_t interval;       // Polling interval for Interrupt/Isochronous
    uint32_t timeout_ms;
    uint64_t submit_tick;
    uint32_t retry_count;
    uint32_t max_retries;
    
    usb_urb_status_t status;
    void* context;
    usb_complete_t complete_cb;
    
    uint32_t ref_count;
    atoms_spinlock_t lock;
    
    struct urb* next;
    struct urb* prev;
} urb_t;

typedef struct {
    urb_t pool[MAX_URB_POOL_SIZE];
    bool in_use[MAX_URB_POOL_SIZE];
    uint32_t active_urbs;
    uint32_t allocated_count;
    uint32_t completed_count;
    uint32_t cancelled_count;
    uint32_t timed_out_count;
    uint32_t retry_count;
    atoms_spinlock_t lock;
} urb_pool_t;

// URB APIs
void usb_urb_engine_init(void);
urb_t* usb_alloc_urb(void);
void usb_free_urb(urb_t* urb);
urb_t* usb_clone_urb(urb_t* src_urb);
void usb_get_urb(urb_t* urb);
void usb_put_urb(urb_t* urb);

bool usb_submit_urb(urb_t* urb);
bool usb_cancel_urb(urb_t* urb);

urb_pool_t* usb_get_urb_pool(void);

#endif // SIGNATURES_USB_URB_H
