/* kernel/net/drivers/realtek/rtl8168.h - Certified RTL8168 1GbE Driver Header */
#ifndef SIGNATURES_RTL8168_H
#define SIGNATURES_RTL8168_H

#include <kernel/net/net_framework.h>

#define RTL8168_REG_MAC0            0x00
#define RTL8168_REG_TX_DESC_LOW     0x20
#define RTL8168_REG_TX_DESC_HIGH    0x24
#define RTL8168_REG_CHIP_CMD        0x37
#define RTL8168_REG_TX_POLL         0x38
#define RTL8168_REG_IMR             0x3C
#define RTL8168_REG_ISR             0x3E
#define RTL8168_REG_TX_CONFIG       0x40
#define RTL8168_REG_RX_CONFIG       0x44
#define RTL8168_REG_9346CR          0x50
#define RTL8168_REG_RMS             0xDA
#define RTL8168_REG_CPLUS_CMD       0xE0
#define RTL8168_REG_RX_DESC_LOW     0xE4
#define RTL8168_REG_RX_DESC_HIGH    0xE8

#define RTL8168_CMD_RESET           0x10
#define RTL8168_CMD_RX_ENABLE       0x08
#define RTL8168_CMD_TX_ENABLE       0x04

#define RTL8168_9346_UNLOCK         0xC0
#define RTL8168_9346_LOCK           0x00

#define RTL8168_RING_SIZE           256

/* RTL8168 16-Byte Descriptor Format */
struct rtl8168_tx_desc {
    volatile uint32_t opts1;     // OWN, EOR, FS, LS, Length
    volatile uint32_t opts2;     // VLAN / Offloads
    volatile uint32_t addr_low;  // Buffer PA Low
    volatile uint32_t addr_high; // Buffer PA High
} __attribute__((packed, aligned(16)));

struct rtl8168_rx_desc {
    volatile uint32_t opts1;     // OWN, EOR, FS, LS, Buffer Size / Packet Length
    volatile uint32_t opts2;     // VLAN / Offloads
    volatile uint32_t addr_low;  // Buffer PA Low
    volatile uint32_t addr_high; // Buffer PA High
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
} rtl8168_private_t;

bool rtl8168_driver_probe(PCIDevice* pdev);

#endif /* SIGNATURES_RTL8168_H */
