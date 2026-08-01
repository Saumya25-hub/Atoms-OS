#include "../include/xhci_interrupt.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/display/display.h"

static xhci_intr_mode_t g_intr_mode = XHCI_INTR_MODE_LEGACY_PIC;
extern volatile uint32_t* g_xhci_db_regs;

void xhci_interrupt_init(void) {
    // In current virtual environment (QEMU / VMware), xHCI legacy PCI IRQ / APIC vector is routed to IRQ 11
    g_intr_mode = XHCI_INTR_MODE_LEGACY_PIC;
    display_print("[XHCI INTR] Interrupt Manager initialized in Legacy PIC/APIC mode.\n");
}

xhci_intr_mode_t xhci_interrupt_get_mode(void) {
    return g_intr_mode;
}

void xhci_doorbell_ring(uint8_t slot_id, uint8_t target_dci) {
    if (!g_xhci_db_regs) return;
    g_xhci_db_regs[slot_id] = target_dci;
}
