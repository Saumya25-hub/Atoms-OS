#ifndef SIGNATURES_E1000_REGS_H
#define SIGNATURES_E1000_REGS_H

#include <stdint.h>

// Intel 82540EM / E1000 Register Offsets
#define E1000_REG_CTRL      0x0000  // Device Control
#define E1000_REG_STATUS    0x0008  // Device Status
#define E1000_REG_EERD      0x0014  // EEPROM Read
#define E1000_REG_ICR       0x00C0  // Interrupt Cause Read
#define E1000_REG_IMS       0x00D0  // Interrupt Mask Set
#define E1000_REG_IMC       0x00D8  // Interrupt Mask Clear

#define E1000_REG_RCTL      0x0100  // Receive Control
#define E1000_REG_RDBAL     0x2800  // RX Descriptor Base Address Low
#define E1000_REG_RDBAH     0x2804  // RX Descriptor Base Address High
#define E1000_REG_RDLEN     0x2808  // RX Descriptor Length
#define E1000_REG_RDH       0x2810  // RX Descriptor Head
#define E1000_REG_RDT       0x2818  // RX Descriptor Tail

#define E1000_REG_TCTL      0x0400  // Transmit Control
#define E1000_REG_TIPG      0x0410  // Transmit Inter-Packet Gap
#define E1000_REG_TDBAL     0x3800  // TX Descriptor Base Address Low
#define E1000_REG_TDBAH     0x3804  // TX Descriptor Base Address High
#define E1000_REG_TDLEN     0x3808  // TX Descriptor Length
#define E1000_REG_TDH       0x3810  // TX Descriptor Head
#define E1000_REG_TDT       0x3818  // TX Descriptor Tail

#define E1000_REG_RAL       0x5400  // Receive Address Low 0
#define E1000_REG_RAH       0x5404  // Receive Address High 0

// Device Control Bits
#define E1000_CTRL_SLU      (1 << 6)    // Set Link Up
#define E1000_CTRL_ASDE     (1 << 5)    // Auto-Speed Detection Enable
#define E1000_CTRL_RST      (1 << 26)   // Device Reset

// Device Status Bits
#define E1000_STATUS_LU     (1 << 1)    // Link Up Status

// Receive Address High Bits
#define E1000_RAH_AV        (1U << 31)  // Address Valid

// EEPROM Read Bits
#define E1000_EERD_START    (1 << 0)    // Start Read
#define E1000_EERD_DONE     (1 << 4)    // Read Done

// Transmit Control Bits
#define E1000_TCTL_EN       (1 << 1)    // Enable Transmit
#define E1000_TCTL_PSP      (1 << 3)    // Pad Short Packets

// Transmit Descriptor Command Bits
#define E1000_TXD_CMD_EOP   (1 << 0)    // End of Packet
#define E1000_TXD_CMD_IFCS  (1 << 1)    // Insert FCS/CRC
#define E1000_TXD_CMD_IC    (1 << 2)    // Insert Checksum
#define E1000_TXD_CMD_RS    (1 << 3)    // Report Status (sets DD bit)

// Transmit Descriptor Status Bits
#define E1000_TXD_STAT_DD   (1 << 0)    // Descriptor Done

// Receive Control Bits
#define E1000_RCTL_EN       (1 << 1)    // Enable Receiver
#define E1000_RCTL_SBP      (1 << 2)    // Store Bad Packets
#define E1000_RCTL_UPE      (1 << 3)    // Unicast Promiscuous Enable
#define E1000_RCTL_MPE      (1 << 4)    // Multicast Promiscuous Enable
#define E1000_RCTL_LPE      (1 << 5)    // Long Packet Enable
#define E1000_RCTL_BAM      (1 << 15)   // Broadcast Accept Mode
#define E1000_RCTL_BSIZE_2048 (0 << 16) // Buffer Size 2048 bytes
#define E1000_RCTL_SECRC    (1 << 26)   // Strip Ethernet CRC

// Receive Descriptor Status Bits
#define E1000_RXD_STAT_DD   (1 << 0)    // Descriptor Done
#define E1000_RXD_STAT_EOP  (1 << 1)    // End of Packet

#endif // SIGNATURES_E1000_REGS_H
