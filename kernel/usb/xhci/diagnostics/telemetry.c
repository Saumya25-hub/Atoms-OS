#include "../include/xhci_debug.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/sync/spinlock.h"

static xhci_telemetry_t g_telemetry;
static atoms_spinlock_t g_telemetry_lock;

void xhci_telemetry_init(void) {
    memset(&g_telemetry, 0, sizeof(xhci_telemetry_t));
    atoms_spinlock_init(&g_telemetry_lock, 1);
}

xhci_telemetry_t* xhci_telemetry_get(void) {
    return &g_telemetry;
}

void xhci_telemetry_record_submit(uint32_t len, bool is_in) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry_lock);
    g_telemetry.transfers_submitted++;
    g_telemetry.trbs_created++;
    if (is_in) g_telemetry.bytes_in += len;
    else g_telemetry.bytes_out += len;
    atoms_spin_unlock_irqrestore(&g_telemetry_lock, state);
}

void xhci_telemetry_record_complete(uint32_t len, bool is_in, uint64_t latency_us) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry_lock);
    g_telemetry.transfers_completed++;
    g_telemetry.total_latency_us += latency_us;
    if (latency_us > g_telemetry.max_latency_us) {
        g_telemetry.max_latency_us = latency_us;
    }
    atoms_spin_unlock_irqrestore(&g_telemetry_lock, state);
}

void xhci_telemetry_record_error(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry_lock);
    g_telemetry.controller_errors++;
    atoms_spin_unlock_irqrestore(&g_telemetry_lock, state);
}

void xhci_telemetry_record_timeout(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry_lock);
    g_telemetry.timeouts++;
    atoms_spin_unlock_irqrestore(&g_telemetry_lock, state);
}
