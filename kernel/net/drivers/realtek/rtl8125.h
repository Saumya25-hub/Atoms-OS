/* kernel/net/drivers/realtek/rtl8125.h - Isolated RTL8125 2.5GbE Extended Driver Header */
#ifndef SIGNATURES_RTL8125_H
#define SIGNATURES_RTL8125_H

#include <kernel/net/net_framework.h>

#define RTL8125_REG_MAC0            0x00
#define RTL8125_REG_TX_DESC_LOW     0x20
#define RTL8125_REG_TX_DESC_HIGH    0x24
#define RTL8125_REG_TX_POLL         0x90  /* RTL8125 TxPoll_8125: 16-bit write BIT(0) */
#define RTL8125_REG_CHIP_CMD        0x37
#define RTL8125_REG_INT_CFG0        0x34
#define RTL8125_REG_IMR             0x38
#define RTL8125_REG_ISR             0x3C
#define RTL8125_REG_TX_CONFIG       0x40
#define RTL8125_REG_RX_CONFIG       0x44
#define RTL8125_REG_INT_CFG1        0x7A
#define RTL8125_REG_OCPDR           0xB0
#define RTL8125_REG_9346CR          0x50
#define RTL8125_REG_RMS             0xDA
#define RTL8125_REG_CPLUS_CMD       0xE0
#define RTL8125_REG_RX_DESC_LOW     0xE4
#define RTL8125_REG_RX_DESC_HIGH    0xE8
#define RTL8125_REG_RSS_CTRL        0x4500
#define RTL8125_REG_QNUM_CTRL       0x4800

#define RTL8125_CMD_RESET           0x10
#define RTL8125_CMD_RX_ENABLE       0x08
#define RTL8125_CMD_TX_ENABLE       0x04

#define RTL8125_RING_SIZE           256

/* RTL8125 uses the standard 16-byte 8169-family descriptor format here. */
struct rtl8125_tx_desc {
    volatile uint32_t opts1;     // OWN (bit 31), EOR (bit 30), FS (bit 29), LS (bit 28), Length
    volatile uint32_t opts2;     // Offloads / MSS / VLAN
    volatile uint32_t addr_low;  // Buffer Physical Address Low
    volatile uint32_t addr_high; // Buffer Physical Address High
} __attribute__((packed, aligned(16)));

struct rtl8125_rx_desc {
    volatile uint32_t opts1;     // OWN, EOR, FS, LS, Length
    volatile uint32_t opts2;     // Offloads / VLAN
    volatile uint32_t addr_low;  // Buffer Physical Address Low
    volatile uint32_t addr_high; // Buffer Physical Address High
} __attribute__((packed, aligned(16)));

typedef struct {
    PCIDevice* pci_device;
    net_device_t* net_dev;
    uint32_t mmio_base;
    uint16_t io_base;
    bool is_mmio;
    uint8_t mac_addr[6];
    uint32_t tx_head;
    uint32_t tx_tail;
    uint32_t rx_head;
} rtl8125_private_t;

bool rtl8125_driver_probe(PCIDevice* pdev);

#endif /* SIGNATURES_RTL8125_H */
