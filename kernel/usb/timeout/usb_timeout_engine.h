#ifndef SIGNATURES_USB_TIMEOUT_ENGINE_H
#define SIGNATURES_USB_TIMEOUT_ENGINE_H

#include "../common/usb_common.h"
#include "../urb/usb_urb.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    uint32_t watchdog_ticks;
    uint32_t timeouts_detected;
    uint32_t retries_triggered;
    uint32_t escalations_triggered;
    atoms_spinlock_t lock;
} usb_timeout_stats_t;

// Timeout Engine APIs
void usb_timeout_engine_init(void);
void usb_timeout_engine_tick(void);

bool usb_timeout_handle_urb(urb_t* urb);

usb_timeout_stats_t* usb_get_timeout_stats(void);

#endif // SIGNATURES_USB_TIMEOUT_ENGINE_H
