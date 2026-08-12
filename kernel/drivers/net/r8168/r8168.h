#ifndef SIGNATURES_R8168_H
#define SIGNATURES_R8168_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/pci/pci.h"

// Realtek 8168/8111 MMIO / IO Register Offsets
#define R8168_REG_MAC0            0x00
#define R8168_REG_TX_DESC_LOW     0x20
#define R8168_REG_TX_DESC_HIGH    0x24
#define R8168_REG_CHIP_CMD        0x37
#define R8168_REG_TX_POLL         0x38
#define R8168_REG_TX_POLL_8125    0x90
#define R8168_REG_IMR             0x3C
#define R8168_REG_ISR             0x3E
#define R8168_REG_IMR_8125        0x38
#define R8168_REG_ISR_8125        0x3C
#define R8168_REG_INT_CFG0_8125   0x34
#define R8168_REG_INT_CFG1_8125   0x7A
#define R8168_REG_OCPDR           0xB0
#define R8168_REG_RSS_CTRL_8125   0x4500
#define R8168_REG_QNUM_CTRL_8125  0x4800
#define R8168_REG_TX_CONFIG       0x40
#define R8168_REG_RX_CONFIG       0x44
#define R8168_REG_9346CR          0x50
#define R8168_REG_RMS             0xDA
#define R8168_REG_CPLUS_CMD       0xE0
#define R8168_REG_RX_DESC_LOW     0xE4
#define R8168_REG_RX_DESC_HIGH    0xE8

// Command Register Bits
#define R8168_CMD_RESET           0x10
#define R8168_CMD_RX_ENABLE       0x08
#define R8168_CMD_TX_ENABLE       0x04

// 9346CR Lock/Unlock Bits
#define R8168_9346_UNLOCK         0xC0
#define R8168_9346_LOCK           0x00

// Realtek 16-byte TX Descriptor Structure
struct r8168_tx_desc {
    volatile uint32_t opts1;     // OWN, EOR, FS, LS, Length
    volatile uint32_t opts2;     // VLAN / Checksum Offloads
    volatile uint32_t addr_low;  // Buffer Low 32-bit Address
    volatile uint32_t addr_high; // Buffer High 32-bit Address
} __attribute__((packed, aligned(16)));

// Realtek 16-byte RX Descriptor Structure
struct r8168_rx_desc {
    volatile uint32_t opts1;     // OWN, EOR, FS, LS, Buffer Size / Frame Length
    volatile uint32_t opts2;     // VLAN / Checksum Offloads
    volatile uint32_t addr_low;  // Buffer Low 32-bit Address
    volatile uint32_t addr_high; // Buffer High 32-bit Address
} __attribute__((packed, aligned(16)));

typedef enum {
    R8168_STATE_UNINITIALIZED = 0,
    R8168_STATE_PCI_FOUND,
    R8168_STATE_READY,
    R8168_STATE_FAILED
} R8168State;

typedef struct {
    PCIDevice* pci_device;
    R8168State state;
    uint32_t mmio_base;
    uint16_t io_base;
    bool is_mmio;
    uint8_t mac_addr[6];
    bool is_rtl8125;
    uint32_t tx_head;   // Producer index: next descriptor to fill
    uint32_t tx_tail;   // Consumer/clean index: next descriptor to reclaim
    uint32_t rx_head;
    uint64_t tx_ok_count;
    uint64_t tx_try_count;
} R8168Device;

void r8168_init(void);
R8168Device* r8168_get_device(void);
bool r8168_transmit_raw(const void* frame, uint16_t length);
void r8168_tx_reclaim(void);   // Reclaim completed TX descriptors (clears OWN=0 entries)
bool r8168_poll_receive(void);

// Live TX diagnostic counters
extern volatile uint64_t g_tx_try_count;
extern volatile uint64_t g_tx_ok_count;
extern volatile uint64_t g_tx_drop_count;
extern volatile uint32_t g_tx_head_snapshot;
extern volatile uint32_t g_tx_tail_snapshot;

uint32_t r8168_get_tx0_opts1(void);
uint8_t r8168_get_chip_cmd(void);
uint16_t r8168_get_isr(void);

typedef struct {
    // PCI Identification Space
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  revision_id;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;

    // PCI Config Space
    uint16_t pci_command;
    uint16_t pci_status;
    uint32_t pci_bar0;
    uint32_t pci_bar1;
    uint8_t  pci_interrupt_line;
    bool     pci_bus_master;
    bool     pci_memory_enable;
    bool     pci_io_enable;

    // Realtek Registers
    uint8_t  chip_cmd;
    uint32_t tx_config;
    uint32_t rx_config;
    uint16_t cplus_cmd;
    uint8_t  tx_poll;
    uint16_t imr;
    uint16_t isr;

    // DMA Base Registers
    uint32_t hw_tx_desc_low;
    uint32_t hw_tx_desc_high;
    uint32_t hw_rx_desc_low;
    uint32_t hw_rx_desc_high;

    // Descriptor Forensics
    uint32_t desc0_opts1;
    uint32_t desc0_opts2;
    uint32_t desc0_addr_low;
    uint32_t desc0_addr_high;

    uint32_t desc1_opts1;
    uint32_t desc1_addr_low;

    uint32_t desc255_opts1;
    uint32_t desc255_addr_low;

    // Address Verification
    uint64_t tx_ring_va;
    uint64_t tx_ring_pa;
    uint64_t rx_ring_va;
    uint64_t rx_ring_pa;
    uint64_t tx_buf0_va;
    uint64_t tx_buf0_pa;
    uint64_t rx_buf0_va;
    uint64_t rx_buf0_pa;

    // DMA Sanity Checks
    bool bus_master_ok;
    bool tx_ring_base_matches;
    bool desc0_addr_matches;
    bool own_cleared_by_hw;
} R8168ForensicReport;

void r8168_run_forensics(R8168ForensicReport *report);
void r8168_print_forensics_serial(const R8168ForensicReport *report);

#endif // SIGNATURES_R8168_H
