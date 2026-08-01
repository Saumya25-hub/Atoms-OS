#ifndef SIGNATURES_UHCI_H
#define SIGNATURES_UHCI_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

// UHCI Registers (I/O Space)
#define UHCI_REG_USBCMD    0x00 // Command Register (16-bit)
#define UHCI_REG_USBSTS    0x02 // Status Register (16-bit)
#define UHCI_REG_USBINTR   0x04 // Interrupt Enable (16-bit)
#define UHCI_REG_FRNUM     0x06 // Frame Number (16-bit)
#define UHCI_REG_FRBASEADD 0x08 // Frame List Base Address (32-bit)
#define UHCI_REG_SOFMOD    0x0C // Start of Frame Modify (8-bit)
#define UHCI_REG_PORTSC1   0x10 // Port 1 Status/Control (16-bit)
#define UHCI_REG_PORTSC2   0x12 // Port 2 Status/Control (16-bit)

// USBCMD Flags
#define UHCI_CMD_RS        (1 << 0) // Run/Stop
#define UHCI_CMD_HCRESET   (1 << 1) // Host Controller Reset
#define UHCI_CMD_GRESET    (1 << 2) // Global Reset
#define UHCI_CMD_MAXP      (1 << 7) // Max Packet 64 bytes

// PORTSC Flags
#define UHCI_PORT_CCS      (1 << 0) // Current Connect Status
#define UHCI_PORT_CSC      (1 << 1) // Connect Status Change
#define UHCI_PORT_PE       (1 << 2) // Port Enable
#define UHCI_PORT_PEC      (1 << 3) // Port Enable Change
#define UHCI_PORT_LS       (3 << 4) // Line Status
#define UHCI_PORT_RD       (1 << 6) // Resume Detect
#define UHCI_PORT_LSDA     (1 << 8) // Low Speed Device Attached
#define UHCI_PORT_PR       (1 << 9) // Port Reset

// UHCI Frame List Pointer Flags
#define UHCI_PTR_TERMINATE (1 << 0)
#define UHCI_PTR_QH        (1 << 1)
#define UHCI_PTR_DEPTH     (1 << 2)

// UHCI Transfer Descriptor (TD) - 16 bytes, 16-byte aligned
typedef struct {
    uint32_t link;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
} __attribute__((packed, aligned(16))) uhci_td_t;

// UHCI Queue Head (QH) - 16 bytes, 16-byte aligned
typedef struct {
    uint32_t head_link;
    uint32_t element_link;
} __attribute__((packed, aligned(16))) uhci_qh_t;

typedef struct {
    uint16_t io_base;
    uint8_t  irq;
    uint32_t* frame_list;       // 1024 dwords (4KB aligned)
    uint64_t  frame_list_phys;
    uhci_qh_t* async_qh;        // Bulk/Control Queue Head
    uhci_qh_t* periodic_qh;     // Interrupt Queue Head
    uint32_t  port_count;
    bool      running;
    atoms_spinlock_t lock;
} uhci_controller_t;

// UHCI APIs
bool uhci_init(uhci_controller_t* udev, uint16_t io_base, uint8_t irq);
void uhci_shutdown(uhci_controller_t* udev);
bool uhci_reset(uhci_controller_t* udev);
bool uhci_port_reset(uhci_controller_t* udev, uint8_t port);
bool uhci_submit_bulk(uhci_controller_t* udev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len);
void uhci_poll(uhci_controller_t* udev);

#endif // SIGNATURES_UHCI_H
