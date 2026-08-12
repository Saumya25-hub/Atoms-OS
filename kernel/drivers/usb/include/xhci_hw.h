#ifndef ATOMS_XHCI_HW_H
#define ATOMS_XHCI_HW_H

#include <stdint.h>
#include <stdbool.h>

#define XHCI_PCI_PROGIF             0x30

// Capability Register Offsets
#define XHCI_CAP_CAPLENGTH          0x00
#define XHCI_CAP_HCIVERSION         0x02
#define XHCI_CAP_HCSPARAMS1         0x04
#define XHCI_CAP_HCSPARAMS2         0x08
#define XHCI_CAP_HCSPARAMS3         0x0C
#define XHCI_CAP_HCCPARAMS1         0x10
#define XHCI_CAP_DBOFF              0x14
#define XHCI_CAP_RTSOFF             0x18

// Operational Register Offsets (relative to OPBASE)
#define XHCI_OP_USBCMD              0x00
#define XHCI_OP_USBSTS              0x04
#define XHCI_OP_PAGESIZE            0x08
#define XHCI_OP_DNCTRL              0x14
#define XHCI_OP_CRCR                0x18
#define XHCI_OP_DCBAAP              0x30
#define XHCI_OP_CONFIG              0x38
#define XHCI_OP_PORTSC_BASE         0x400

// TRB Types
#define XHCI_TRB_NORMAL             1
#define XHCI_TRB_SETUP_STAGE        2
#define XHCI_TRB_DATA_STAGE         3
#define XHCI_TRB_STATUS_STAGE       4
#define XHCI_TRB_ISOCH              5
#define XHCI_TRB_LINK               6
#define XHCI_TRB_EVENT_DATA         7
#define XHCI_TRB_NO_OP              8

#define XHCI_TRB_ENABLE_SLOT        9
#define XHCI_TRB_DISABLE_SLOT       10
#define XHCI_TRB_ADDRESS_DEVICE     11
#define XHCI_TRB_CONFIGURE_ENDPOINT 12
#define XHCI_TRB_EVALUATE_CONTEXT   13
#define XHCI_TRB_RESET_ENDPOINT     14
#define XHCI_TRB_STOP_ENDPOINT      15
#define XHCI_TRB_SET_TR_DEQUEUE_PTR 16
#define XHCI_TRB_RESET_DEVICE       17

#define XHCI_TRB_EVENT_TRANSFER     32
#define XHCI_TRB_EVENT_CMD_COMPLETE 33
#define XHCI_TRB_EVENT_PORT_STATUS  34

// Standard 16-byte Transfer Request Block (TRB)
typedef struct {
    uint32_t parameter_lo;
    uint32_t parameter_hi;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) XHCI_TRB;

// Event Ring Segment Table Entry (ERST Entry)
typedef struct {
    uint64_t ring_segment_base_address;
    uint16_t ring_segment_size;
    uint16_t reserved1;
    uint32_t reserved2;
} __attribute__((packed)) XHCI_ERST_ENTRY;

#endif // ATOMS_XHCI_HW_H
