#ifndef SIGNATURES_OHCI_H
#define SIGNATURES_OHCI_H

#include "../common/usb_common.h"
#include "kernel/core/sync/spinlock.h"

// OHCI MMIO Register Offsets
#define OHCI_REG_REVISION         0x00
#define OHCI_REG_CONTROL          0x04
#define OHCI_REG_CMD_STATUS       0x08
#define OHCI_REG_INTR_STATUS      0x0C
#define OHCI_REG_INTR_ENABLE      0x10
#define OHCI_REG_HCCA             0x18
#define OHCI_REG_PERIODIC_START   0x1C
#define OHCI_REG_CONTROL_HEAD_ED  0x20
#define OHCI_REG_BULK_HEAD_ED     0x28
#define OHCI_REG_DONE_HEAD        0x2C
#define OHCI_REG_RH_DESCRIPTOR_A  0x48
#define OHCI_REG_RH_PORT_STATUS   0x54

// OHCI HCCA Structure (256-byte aligned)
typedef struct {
    uint32_t HccaInterruptTable[32];
    uint16_t HccaFrameNumber;
    uint16_t HccaPad1;
    uint32_t HccaDoneHead;
    uint8_t  reserved[116];
} __attribute__((packed, aligned(256))) ohci_hcca_t;

// OHCI Endpoint Descriptor (ED) - 16 bytes, 16-byte aligned
typedef struct {
    uint32_t flags;
    uint32_t tail_p;
    uint32_t head_p;
    uint32_t next_ed;
} __attribute__((packed, aligned(16))) ohci_ed_t;

// OHCI Transfer Descriptor (TD) - 16 bytes, 16-byte aligned
typedef struct {
    uint32_t flags;
    uint32_t cbp; // Current Buffer Pointer
    uint32_t next_td;
    uint32_t be;  // Buffer End
} __attribute__((packed, aligned(16))) ohci_td_t;

typedef struct {
    uint64_t mmio_base;
    uint8_t  irq;
    ohci_hcca_t* hcca;
    uint64_t     hcca_phys;
    ohci_ed_t*   control_head;
    ohci_ed_t*   bulk_head;
    uint32_t     port_count;
    bool         running;
    atoms_spinlock_t lock;
} ohci_controller_t;

// OHCI APIs
bool ohci_init(ohci_controller_t* odev, uint64_t mmio_base, uint8_t irq);
void ohci_shutdown(ohci_controller_t* odev);
bool ohci_reset(ohci_controller_t* odev);
bool ohci_port_reset(ohci_controller_t* odev, uint8_t port);
bool ohci_submit_bulk(ohci_controller_t* odev, uint8_t dev_addr, uint8_t ep, bool dir_in, void* buf, uint32_t len);
void ohci_poll(ohci_controller_t* odev);

#endif // SIGNATURES_OHCI_H
