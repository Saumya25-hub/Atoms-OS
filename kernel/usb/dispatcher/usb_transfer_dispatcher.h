#ifndef SIGNATURES_USB_TRANSFER_DISPATCHER_H
#define SIGNATURES_USB_TRANSFER_DISPATCHER_H

#include "../common/usb_common.h"
#include "../urb/usb_urb.h"
#include "../controller/usb_controller_manager.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    uint32_t total_dispatched;
    uint32_t uhci_dispatched;
    uint32_t ohci_dispatched;
    uint32_t ehci_dispatched;
    uint32_t xhci_dispatched;
    uint32_t failed_dispatches;
    atoms_spinlock_t lock;
} usb_dispatcher_stats_t;

// Transfer Dispatcher APIs
void usb_transfer_dispatcher_init(void);

bool usb_dispatch_urb(urb_t* urb);
bool usb_dispatch_to_controller(usb_controller_type_t ctrl_type, uint32_t ctrl_id, urb_t* urb);

usb_dispatcher_stats_t* usb_get_dispatcher_stats(void);

#endif // SIGNATURES_USB_TRANSFER_DISPATCHER_H
