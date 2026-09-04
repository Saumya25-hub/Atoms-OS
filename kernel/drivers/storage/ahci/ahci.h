#ifndef ATOMS_AHCI_H
#define ATOMS_AHCI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/pci/pci.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

/* =========================================================================
 * AHCI Global Register Offsets (Generic Host Control)
 * ========================================================================= */
#define AHCI_REG_CAP        0x00    /* Host Capabilities */
#define AHCI_REG_GHC        0x04    /* Global Host Control */
#define AHCI_REG_IS         0x08    /* Interrupt Status */
#define AHCI_REG_PI         0x0C    /* Ports Implemented (32-bit bitmask) */
#define AHCI_REG_VS         0x10    /* AHCI Version (Major.Minor) */
#define AHCI_REG_CCC_CTL    0x14    /* Command Completion Coalescing Control */
#define AHCI_REG_CCC_PORTS  0x18    /* Command Completion Coalescing Ports */
#define AHCI_REG_EM_LOC     0x1C    /* Enclosure Management Location */
#define AHCI_REG_EM_CTL     0x20    /* Enclosure Management Control */
#define AHCI_REG_CAP2       0x24    /* Extended Host Capabilities */
#define AHCI_REG_BOHC       0x28    /* BIOS/OS Handoff Control and Status */

/* GHC Bits */
#define AHCI_GHC_HR         (1U << 0)   /* HBA Reset */
#define AHCI_GHC_IE         (1U << 1)   /* Interrupt Enable */
#define AHCI_GHC_MRSM       (1U << 2)   /* MSI Revert to Single Message */
#define AHCI_GHC_AE         (1U << 31)  /* AHCI Enable */

/* CAP2 / BOHC Bits (BIOS/OS Handoff) */
#define AHCI_CAP2_BOH       (1U << 0)   /* BIOS/OS Handoff Supported */
#define AHCI_BOHC_BOS       (1U << 0)   /* BIOS Owned Semaphore */
#define AHCI_BOHC_OOS       (1U << 1)   /* OS Owned Semaphore */
#define AHCI_BOHC_SOOE      (1U << 2)   /* SMI on OS Ownership Change Enable */
#define AHCI_BOHC_OOC       (1U << 3)   /* OS Ownership Change */
#define AHCI_BOHC_BB        (1U << 4)   /* BIOS Busy */

/* =========================================================================
 * AHCI Port Register Offsets (relative to Port Base: 0x100 + port * 0x80)
 * ========================================================================= */
#define AHCI_PORT_CLB       0x00    /* Command List Base Address (1KB aligned) */
#define AHCI_PORT_CLBU      0x04    /* Command List Base Address Upper 32 Bits */
#define AHCI_PORT_FB        0x08    /* FIS Base Address (256B aligned) */
#define AHCI_PORT_FBU       0x0C    /* FIS Base Address Upper 32 Bits */
#define AHCI_PORT_IS        0x10    /* Interrupt Status */
#define AHCI_PORT_IE        0x14    /* Interrupt Enable */
#define AHCI_PORT_CMD       0x18    /* Command and Status */
#define AHCI_PORT_TFD       0x20    /* Task File Data */
#define AHCI_PORT_SIG       0x24    /* Signature */
#define AHCI_PORT_SSTS      0x28    /* Serial ATA Status (SCR0: SStatus) */
#define AHCI_PORT_SCTL      0x2C    /* Serial ATA Control (SCR2: SControl) */
#define AHCI_PORT_SERR      0x30    /* Serial ATA Error (SCR1: SError) */
#define AHCI_PORT_SACT      0x34    /* Serial ATA Active (SCR3: SActive) */
#define AHCI_PORT_CI        0x38    /* Command Issue */
#define AHCI_PORT_SNTF      0x3C    /* Serial ATA Notification (SCR4: SNotification) */
#define AHCI_PORT_FBS       0x40    /* FIS-based Switching Control */

/* Port Command Register Bits (PxCMD) */
#define AHCI_PxCMD_ST       (1U << 0)   /* Start processing command list */
#define AHCI_PxCMD_SUD      (1U << 1)   /* Spin-Up Device */
#define AHCI_PxCMD_POD      (1U << 2)   /* Power On Device */
#define AHCI_PxCMD_CLO      (1U << 3)   /* Command List Override */
#define AHCI_PxCMD_FRE      (1U << 4)   /* FIS Receive Enable */
#define AHCI_PxCMD_CCS_MASK (0x1FU << 8)/* Current Command Slot */
#define AHCI_PxCMD_MPSS     (1U << 13)  /* Mechanical Presence Switch State */
#define AHCI_PxCMD_FR       (1U << 14)  /* FIS Receive Running */
#define AHCI_PxCMD_CR       (1U << 15)  /* Command List Running */
#define AHCI_PxCMD_ATAPI    (1U << 24)  /* Device is ATAPI */

/* Port Task File Data Bits (PxTFD) */
#define AHCI_PxTFD_ERR      (1U << 0)   /* Error bit */
#define AHCI_PxTFD_DRQ      (1U << 3)   /* Data Request bit */
#define AHCI_PxTFD_BSY      (1U << 7)   /* Busy bit */

/* Port SATA Status Bits (PxSSTS) */
#define AHCI_PxSSTS_DET_MASK        0x0F
#define AHCI_PxSSTS_DET_NONE        0x00    /* No device detected, no Phy comm */
#define AHCI_PxSSTS_DET_PRESENT     0x03    /* Device detected, Phy comm established */
#define AHCI_PxSSTS_IPM_MASK        (0x0F << 8)
#define AHCI_PxSSTS_IPM_ACTIVE      (0x01 << 8) /* Interface in active power state */

/* SATA Device Signatures (PxSIG) */
#define SATA_SIG_ATA        0x00000101ULL   /* SATA Drive (HDD / SSD) */
#define SATA_SIG_ATAPI      0xEB140101ULL   /* SATAPI Optical Drive (CD/DVD) */
#define SATA_SIG_SEMB       0xC33C0101ULL   /* Enclosure Management Bridge */
#define SATA_SIG_PM         0x96690101ULL   /* Port Multiplier */

/* ATA Command Opcodes */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_IDENTIFY_PACKET 0xA1

/* FIS Types */
#define FIS_TYPE_REG_H2D        0x27    /* Register FIS - Host to Device */
#define FIS_TYPE_REG_D2H        0x34    /* Register FIS - Device to Host */
#define FIS_TYPE_DMA_ACT        0x39    /* DMA Activate FIS */
#define FIS_TYPE_DMA_SETUP      0x41    /* DMA Setup FIS */
#define FIS_TYPE_DATA           0x46    /* Data FIS */
#define FIS_TYPE_BIST           0x58    /* BIST Activate FIS */
#define FIS_TYPE_PIO_SETUP      0x5F    /* PIO Setup FIS */
#define FIS_TYPE_DEV_BITS       0xA1    /* Set Device Bits FIS */

/* Limits */
#define MAX_AHCI_PORTS          32
#define MAX_AHCI_DRIVES         8

/* =========================================================================
 * Memory Data Structures (Aligned DMA Tables)
 * ========================================================================= */

/* Command Header (32 bytes per slot) */
typedef struct __attribute__((packed)) {
    uint8_t  cfl:5;         /* Command FIS Length in DWORDS (5 for RegH2D) */
    uint8_t  a:1;           /* ATAPI */
    uint8_t  w:1;           /* Write (1 = to device, 0 = from device) */
    uint8_t  p:1;           /* Prefetchable */
    uint8_t  r:1;           /* Reset */
    uint8_t  b:1;           /* BIST */
    uint8_t  c:1;           /* Clear Busy upon R_OK */
    uint8_t  rsvd0:1;
    uint8_t  pmp:4;         /* Port Multiplier Port */
    uint16_t prdtl;         /* PRD Table Length (number of PRD entries) */

    volatile uint32_t prdbc;/* PRD Byte Count transferred */

    uint32_t ctba;          /* Command Table Descriptor Base Address (Bits 31:7, 128B aligned) */
    uint32_t ctbau;         /* Command Table Descriptor Base Address Upper 32 Bits */

    uint32_t rsvd1[4];      /* Reserved */
} AHCICommandHeader;

/* Physical Region Descriptor Table (PRDT) Entry (16 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t dba;           /* Data Base Address (Bits 31:1, Bit 0 = 0) */
    uint32_t dbau;          /* Data Base Address Upper 32 Bits */
    uint32_t rsvd0;         /* Reserved */
    uint32_t dbc:22;        /* Data Byte Count (0-indexed: byte_count - 1) */
    uint32_t rsvd1:9;       /* Reserved */
    uint32_t i:1;           /* Interrupt on Completion (1 = trigger) */
} AHCIPRDTEntry;

/* Command Table (Pointed to by ctba in Command Header) */
typedef struct __attribute__((packed)) {
    uint8_t  cfis[64];      /* Command FIS (up to 64 bytes) */
    uint8_t  acmd[16];      /* ATAPI Command (16 bytes) */
    uint8_t  rsvd[48];      /* Reserved (48 bytes) */
    AHCIPRDTEntry prdt_entries[16]; /* Up to 16 PRD entries (256 bytes) */
} AHCICommandTable;

/* Received FIS Area (256 bytes per port) */
typedef struct __attribute__((packed)) {
    uint8_t dsfis[0x1C];    /* DMA Setup FIS */
    uint8_t rsvd0[0x04];
    uint8_t psfis[0x14];    /* PIO Setup FIS */
    uint8_t rsvd1[0x0C];
    uint8_t rfis[0x14];     /* D2H Register FIS */
    uint8_t rsvd2[0x04];
    uint8_t sdbfis[0x08];   /* Set Device Bits FIS */
    uint8_t ufis[0x40];     /* Unknown FIS */
    uint8_t rsvd3[0x60];    /* Reserved */
} AHCIFISReceived;

/* Host-to-Device Register FIS (FIS Type 0x27) */
typedef struct __attribute__((packed)) {
    uint8_t  fis_type;      /* FIS_TYPE_REG_H2D = 0x27 */
    uint8_t  pm:4;          /* Port multiplier */
    uint8_t  rsvd0:3;       /* Reserved */
    uint8_t  c:1;           /* Command: 1 = Command Register write, 0 = Control */
    uint8_t  command;       /* ATA Command opcode */
    uint8_t  feature_low;   /* Feature Low */

    uint8_t  lba0;          /* LBA Bits 7:0 */
    uint8_t  lba1;          /* LBA Bits 15:8 */
    uint8_t  lba2;          /* LBA Bits 23:16 */
    uint8_t  device;        /* Device Register (bit 6 = 1 for LBA mode) */

    uint8_t  lba3;          /* LBA Bits 31:24 */
    uint8_t  lba4;          /* LBA Bits 39:32 */
    uint8_t  lba5;          /* LBA Bits 47:40 */
    uint8_t  feature_high;  /* Feature High */

    uint8_t  count_low;     /* Sector Count Bits 7:0 */
    uint8_t  count_high;    /* Sector Count Bits 15:8 */
    uint8_t  icc;           /* Isochronous Command Completion */
    uint8_t  control;       /* Control Register */

    uint8_t  rsvd1[4];      /* Reserved */
} AHCIFIS_RegH2D;

/* Context structure for identified SATA drives */
typedef struct {
    uint8_t  port_num;
    uint32_t sector_size;
    uint64_t sector_count;
    char     model[41];
    char     serial[21];
} AHCIDriveData;

/* Driver Lifecycle APIs */
bool ahci_init(void);
int ahci_get_drive_count(void);
AHCIDriveData* ahci_get_drive_data(int index);
bool ahci_read_sectors(uint8_t port, uint64_t lba, uint32_t count, void* buffer);

#endif /* ATOMS_AHCI_H */
