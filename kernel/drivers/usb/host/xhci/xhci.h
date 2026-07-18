#ifndef SIGNATURES_XHCI_H
#define SIGNATURES_XHCI_H

#include <stdint.h>

#define XHCI_PCI_PROGIF 0x30

// PCI Command Register Offsets & Bits
#define PCI_COMMAND_OFFSET 0x04
#define PCI_COMMAND_MEMORY (1 << 1)
#define PCI_COMMAND_MASTER (1 << 2)

// Initialize xHCI Host Controller
void xhci_init(void);

#endif // SIGNATURES_XHCI_H
