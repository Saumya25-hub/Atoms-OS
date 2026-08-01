#include "usb_resource_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static usb_resource_stats_t g_res_stats;

void usb_resource_manager_init(void) {
    memset(&g_res_stats, 0, sizeof(usb_resource_stats_t));
    atoms_spinlock_init(&g_res_stats.lock, 1);
    display_print("[USB RESOURCE] Resource Tracking & Memory Pool Manager Initialized.\n");
}

void usb_resource_track_dma(size_t bytes) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_res_stats.lock);
    g_res_stats.total_dma_bytes_allocated += bytes;
    atoms_spin_unlock_irqrestore(&g_res_stats.lock, state);
}

void usb_resource_untrack_dma(size_t bytes) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_res_stats.lock);
    if (g_res_stats.total_dma_bytes_allocated >= bytes) {
        g_res_stats.total_dma_bytes_allocated -= bytes;
    }
    atoms_spin_unlock_irqrestore(&g_res_stats.lock, state);
}

void usb_resource_track_bounce_alloc(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_res_stats.lock);
    g_res_stats.bounce_buffers_created++;
    atoms_spin_unlock_irqrestore(&g_res_stats.lock, state);
}

void usb_resource_track_bounce_free(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_res_stats.lock);
    g_res_stats.bounce_buffers_freed++;
    atoms_spin_unlock_irqrestore(&g_res_stats.lock, state);
}

usb_resource_stats_t* usb_get_resource_stats(void) {
    return &g_res_stats;
}
