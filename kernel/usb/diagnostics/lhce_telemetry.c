#include "lhce_telemetry.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static lhce_telemetry_t g_telemetry;

void lhce_telemetry_init(void) {
    memset(&g_telemetry, 0, sizeof(lhce_telemetry_t));
    atoms_spinlock_init(&g_telemetry.lock, 1);
}

void lhce_telemetry_record_submit(size_t len, bool is_in) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry.lock);
    g_telemetry.total_transfers_submitted++;
    if (is_in) g_telemetry.total_bytes_in += len;
    else g_telemetry.total_bytes_out += len;
    atoms_spin_unlock_irqrestore(&g_telemetry.lock, state);
}

void lhce_telemetry_record_complete(size_t len, bool is_in) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry.lock);
    (void)len; (void)is_in;
    g_telemetry.total_transfers_completed++;
    atoms_spin_unlock_irqrestore(&g_telemetry.lock, state);
}

void lhce_telemetry_record_timeout(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry.lock);
    g_telemetry.total_timeouts++;
    atoms_spin_unlock_irqrestore(&g_telemetry.lock, state);
}

void lhce_telemetry_record_reset(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry.lock);
    g_telemetry.total_resets++;
    atoms_spin_unlock_irqrestore(&g_telemetry.lock, state);
}

void lhce_telemetry_record_interrupt(void) {
    atoms_irq_lock_state_t state = atoms_spin_lock_irqsave(&g_telemetry.lock);
    g_telemetry.total_interrupts++;
    atoms_spin_unlock_irqrestore(&g_telemetry.lock, state);
}
