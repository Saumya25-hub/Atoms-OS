#ifndef SIGNATURES_USB_COMMON_H
#define SIGNATURES_USB_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/sync/spinlock.h"

// USB Controller Types & Specification Standards
typedef enum {
    USB_CONTROLLER_TYPE_UNKNOWN = 0,
    USB_CONTROLLER_TYPE_UHCI,   // USB 1.1 Intel (Prog IF 0x00)
    USB_CONTROLLER_TYPE_OHCI,   // USB 1.1 AMD/VIA (Prog IF 0x10)
    USB_CONTROLLER_TYPE_EHCI,   // USB 2.0 High-Speed (Prog IF 0x20)
    USB_CONTROLLER_TYPE_XHCI    // USB 3.x SuperSpeed (Prog IF 0x30)
} usb_controller_type_t;

// USB Device Speeds
typedef enum {
    USB_SPEED_LOW = 0,   // 1.5 Mbps (USB 1.0/1.1)
    USB_SPEED_FULL,      // 12 Mbps  (USB 1.1)
    USB_SPEED_HIGH,      // 480 Mbps (USB 2.0)
    USB_SPEED_SUPER,     // 5 Gbps   (USB 3.0)
    USB_SPEED_SUPER_PLUS // 10+ Gbps (USB 3.1/3.2)
} usb_speed_t;

// USB Endpoint Transfer Types
typedef enum {
    USB_TRANSFER_TYPE_CONTROL = 0,
    USB_TRANSFER_TYPE_ISOCHRONOUS,
    USB_TRANSFER_TYPE_BULK,
    USB_TRANSFER_TYPE_INTERRUPT
} usb_transfer_type_t;

// USB Transfer Direction
typedef enum {
    USB_DIR_OUT = 0,
    USB_DIR_IN = 1
} usb_direction_t;

// Common Device Structure
typedef struct {
    uint8_t  slot_or_addr;
    uint8_t  port;
    usb_speed_t speed;
    uint16_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
} usb_device_info_t;

#endif // SIGNATURES_USB_COMMON_H
