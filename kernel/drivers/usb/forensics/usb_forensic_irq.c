#include "usb_forensic_center.h"

void usb_forensic_log_interrupt(uint64_t irq_tick) {
    g_forensic_center.interrupts.irq_count++;
    g_forensic_center.interrupts.last_irq_timestamp = irq_tick;
}
