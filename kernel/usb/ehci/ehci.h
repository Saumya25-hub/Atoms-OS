#ifndef SIGNATURES_EHCI_H
#define SIGNATURES_EHCI_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

// EHCI Operational Register Offsets (relative to op_base)
#define EHCI_REG_USBCMD          0x00 // USB Command (32-bit)
#define EHCI_REG_USBSTS          0x04 // USB Status (32-bit)
#define EHCI_REG_USBINTR         0x08 // USB Interrupt Enable (32-bit)
#define EHCI_REG_FRINDEX         0x0C // Frame Index (32-bit)
#define EHCI_REG_CTRLDSSEGMENT   0x10 // Control Data Structure Segment (4GB)
#define EHCI_REG_PERIODICLISTBASE 0x14 // Periodic Frame List Base Address
#define EHCI_REG_ASYNCLISTADDR   0x18 // Async List Address
#define EHCI_REG_CONFIGFLAG      0x40 // Configured Flag Register
#define EHCI_REG_PORTSC1         0x44 // Port Status/Control 1

// USBCMD Flags
#define EHCI_CMD_RS              (1 << 0) // Run/Stop
#define EHCI_CMD_HCRESET         (1 << 1) // Host Controller Reset
#define EHCI_CMD_PSE             (1 << 4) // Periodic Schedule Enable
#define EHCI_CMD_ASE             (1 << 5) // Async Schedule Enable
#define EHCI_CMD_IAAD            (1 << 6) // Interrupt on Async Advance Doorbell

// USBSTS Flags
#define EHCI_STS_USBINT          (1 << 0) // USB Interrupt
#define EHCI_STS_ERROR           (1 << 1) // USB Error Interrupt
#define EHCI_STS_PORTCHANGE      (1 << 2) // Port Change Detect
#define EHCI_STS_HALTED          (1 << 12) // HC Halted
#define EHCI_STS_ASYNCH_STATUS   (1 << 15) // Async Schedule Status

// PORTSC Flags
#define EHCI_PORT_CCS            (1 << 0) // Current Connect Status
#define EHCI_PORT_CSC            (1 << 1) // Connect Status Change
#define EHCI_PORT_PE             (1 << 2) // Port Enable
#define EHCI_PORT_PEC            (1 << 3) // Port Enable Change
#define EHCI_PORT_OVERCURRENT    (1 << 4) // Over-current Active
#define EHCI_PORT_PR             (1 << 8) // Port Reset
#define EHCI_PORT_POWER          (1 << 12) // Port Power
#define EHCI_PORT_OWNER          (1 << 13) // Port Owner (1=Companion UHCI/OHCI)

// EHCI Queue Transfer Descriptor (qTD) - 32 bytes, 32-byte aligned
typedef struct {
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed, aligned(32))) ehci_qtd_t;

// EHCI Queue Head (QH) - 64 bytes, 64-byte aligned
typedef struct {
    uint32_t qh_link;
    uint32_t ep_char;
    uint32_t ep_caps;
    uint32_t current_qtd;
    ehci_qtd_t overlay;
} __attribute__((packed, aligned(64))) ehci_qh_t;

typedef struct {
    uint64_t mmio_base;
    uint64_t op_base;
    uint8_t  cap_length;
    uint8_t  irq;
    ehci_qh_t* async_qh;        // Async Schedule Head Pointer
    uint64_t   async_qh_phys;
    uint32_t*  periodic_list;   // 1024-entry Periodic Frame List
    uint64_t   periodic_phys;
    uint32_t   port_count;
    bool       running;
    atoms_spinlock_t lock;
} ehci_controller_t;

// EHCI APIs
bool ehci_init(ehci_controller_t* edev, uint64_t mmio_base, uint8_t irq);
void ehci_shutdown(ehci_controller_t* edev);
bool ehci_reset(ehci_controller_t* edev);
bool ehci_port_reset(ehci_controller_t* edev, uint8_t port);
bool ehci_submit_bulk(ehci_controller_t* edev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len);
void ehci_poll(ehci_controller_t* edev);

#endif // SIGNATURES_EHCI_H
