#ifndef SIGNATURES_USB_COMPLETION_ENGINE_H
#define SIGNATURES_USB_COMPLETION_ENGINE_H

#include "../common/usb_common.h"
#include "../urb/usb_urb.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    uint32_t total_completions;
    uint32_t sync_completions;
    uint32_t async_completions;
    uint32_t callbacks_dispatched;
    atoms_spinlock_t lock;
} usb_completion_stats_t;

// Completion Engine APIs
void usb_completion_engine_init(void);

void usb_complete_urb(urb_t* urb, usb_urb_status_t status, uint32_t actual_length);
bool usb_wait_for_urb(urb_t* urb, uint32_t timeout_ms);

usb_completion_stats_t* usb_get_completion_stats(void);

#endif // SIGNATURES_USB_COMPLETION_ENGINE_H
