#ifndef SIGNATURES_XHCI_INTERRUPT_H
#define SIGNATURES_XHCI_INTERRUPT_H

#include <stdint.h>
#include <stdbool.h>

// Interrupt Modes
typedef enum {
    XHCI_INTR_MODE_LEGACY_PIC = 0,
    XHCI_INTR_MODE_MSI,
    XHCI_INTR_MODE_MSIX
} xhci_intr_mode_t;

// API Declarations
void xhci_interrupt_init(void);
xhci_intr_mode_t xhci_interrupt_get_mode(void);
void xhci_doorbell_ring(uint8_t slot_id, uint8_t target_dci);
void xhci_interrupt_handler_main(void);

#endif // SIGNATURES_XHCI_INTERRUPT_H
