#ifndef SIGNATURES_USB_ENDPOINT_H
#define SIGNATURES_USB_ENDPOINT_H

#include "../common/usb_common.h"
#include "../core/usb_core.h"
#include "kernel/core/sync/spinlock.h"

#define MAX_ENDPOINTS_PER_DEVICE 32

typedef enum {
    USB_EP_STATE_IDLE = 0,
    USB_EP_STATE_RUNNING,
    USB_EP_STATE_STALL,
    USB_EP_STATE_NAK,
    USB_EP_STATE_HALT,
    USB_EP_STATE_RESET,
    USB_EP_STATE_ERROR
} usb_endpoint_state_t;

typedef struct {
    uint8_t ep_address;      // Includes direction bit (0x80 for IN)
    uint8_t ep_number;       // Endpoint number 0-15
    usb_transfer_type_t type;
    usb_direction_t dir;
    usb_endpoint_state_t state;
    
    uint16_t max_packet_size;
    uint8_t interval;        // Polling interval
    uint8_t toggle;          // Data toggle bit (0 or 1)
    
    uint32_t active_urbs;
    uint32_t total_transfers;
    uint32_t error_count;
    
    usb_device_t* dev;
    atoms_spinlock_t lock;
} usb_endpoint_t;

typedef struct {
    usb_endpoint_t endpoints[MAX_ENDPOINTS_PER_DEVICE];
    uint32_t count;
    atoms_spinlock_t lock;
} usb_endpoint_table_t;

// Endpoint Manager APIs
void usb_endpoint_manager_init(void);
usb_endpoint_t* usb_endpoint_create(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir, usb_transfer_type_t type, uint16_t max_packet_size, uint8_t interval);
usb_endpoint_t* usb_endpoint_get(usb_device_t* dev, uint8_t ep_num, usb_direction_t dir);

bool usb_endpoint_set_state(usb_endpoint_t* ep, usb_endpoint_state_t state);
usb_endpoint_state_t usb_endpoint_get_state(usb_endpoint_t* ep);

void usb_endpoint_reset_toggle(usb_endpoint_t* ep);
uint8_t usb_endpoint_get_toggle(usb_endpoint_t* ep);
void usb_endpoint_advance_toggle(usb_endpoint_t* ep);

bool usb_endpoint_halt(usb_endpoint_t* ep);
bool usb_endpoint_clear_halt(usb_endpoint_t* ep);
bool usb_endpoint_recover(usb_endpoint_t* ep);

#endif // SIGNATURES_USB_ENDPOINT_H
