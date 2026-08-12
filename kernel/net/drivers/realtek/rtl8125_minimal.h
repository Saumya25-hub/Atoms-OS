#ifndef RTL8125_MINIMAL_H
#define RTL8125_MINIMAL_H

#include <stdint.h>
#include <kernel/core/pci/pci.h>

// ============================================================================
// NO GUESSWORK RULE: Exact RTL8125 Register Offsets
// ============================================================================

// MAC Address base (6 bytes)
#define RTL8125_REG_MAC0            0x00

// Transmit Descriptor Base Physical Address (64-bit)
#define RTL8125_REG_TX_DESC_LOW     0x20
#define RTL8125_REG_TX_DESC_HIGH    0x24

// Chip Command Register (Bit 3 = TX Enable, Bit 2 = RX Enable)
#define RTL8125_REG_CHIP_CMD        0x37
#define RTL8125_CMD_RESET           0x10
#define RTL8125_CMD_TX_EN           0x08
#define RTL8125_CMD_RX_EN           0x04

// Transmit Configuration Register
// Linux writes 0x03000700 (standard DMA burst and interframe gap)
#define RTL8125_REG_TX_CONFIG       0x40
#define RTL8125_TX_CONFIG_DEFAULT   0x03000700

// 9346 Config Register (Unlock MAC/Config registers)
#define RTL8125_REG_CFG9346         0x50
#define RTL8125_CFG9346_UNLOCK      0xC0
#define RTL8125_CFG9346_LOCK        0x00

// ----------------------------------------------------------------------------
// THE CRUX: TxNoClose Doorbell (Realtek RTL8125B/BG Rev 05)
// Realtek Linux r8125 driver uses this instead of 0x90 polling.
// Write the actual `cur_tx` descriptor index here (16-bit or 32-bit).
// ----------------------------------------------------------------------------
#define RTL8125_SW_TAIL_PTR0        0x2800


// ============================================================================
// STRICT 16-BYTE TX DESCRIPTOR (Legacy Compatible)
// ============================================================================
// Do NOT use 32-byte extended descriptors to ensure minimum complexity.
typedef struct {
    uint32_t opts1; // Command/Status: OWN, FS, LS, Length
    uint32_t opts2; // VLAN info (usually 0)
    uint64_t addr;  // Physical address of packet buffer
} __attribute__((packed)) rtl8125_min_desc_t;

// Descriptor Command Bits
#define RTL8125_DESC_OWN            0x80000000 // Ownership (1 = NIC, 0 = Host)
#define RTL8125_DESC_EOR            0x40000000 // End of Ring
#define RTL8125_DESC_FS             0x20000000 // First Segment
#define RTL8125_DESC_LS             0x10000000 // Last Segment

// ----------------------------------------------------------------------------
// Minimal Test Routine Hook
// ----------------------------------------------------------------------------
void rtl8125_minimal_test(PCIDevice* dev);

#endif
