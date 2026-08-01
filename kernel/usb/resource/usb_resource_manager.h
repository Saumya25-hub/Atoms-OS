#ifndef SIGNATURES_USB_RESOURCE_MANAGER_H
#define SIGNATURES_USB_RESOURCE_MANAGER_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

typedef struct {
    uint32_t active_urbs;
    uint32_t active_pipes;
    uint32_t active_endpoints;
    uint32_t active_devices;
    uint64_t total_dma_bytes_allocated;
    uint32_t bounce_buffers_created;
    uint32_t bounce_buffers_freed;
    atoms_spinlock_t lock;
} usb_resource_stats_t;

// Resource Manager APIs
void usb_resource_manager_init(void);

void usb_resource_track_dma(size_t bytes);
void usb_resource_untrack_dma(size_t bytes);
void usb_resource_track_bounce_alloc(void);
void usb_resource_track_bounce_free(void);

usb_resource_stats_t* usb_get_resource_stats(void);

#endif // SIGNATURES_USB_RESOURCE_MANAGER_H
