#ifndef ATOMS_NVME_H
#define ATOMS_NVME_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

/* NVMe Register Offsets (relative to BAR0) */
#define NVME_REG_CAP        0x0000  /* Controller Capabilities (64-bit) */
#define NVME_REG_VS         0x0008  /* Version (32-bit) */
#define NVME_REG_INTMS      0x000C  /* Interrupt Mask Set (32-bit) */
#define NVME_REG_INTMC      0x0010  /* Interrupt Mask Clear (32-bit) */
#define NVME_REG_CC         0x0014  /* Controller Configuration (32-bit) */
#define NVME_REG_CSTS       0x001C  /* Controller Status (32-bit) */
#define NVME_REG_NSSR       0x0020  /* NVM Subsystem Reset (32-bit) */
#define NVME_REG_AQA        0x0024  /* Admin Queue Attributes (32-bit) */
#define NVME_REG_ASQ        0x0028  /* Admin Submission Queue Base (64-bit) */
#define NVME_REG_ACQ        0x0030  /* Admin Completion Queue Base (64-bit) */

/* CC (Controller Configuration) Bitfields */
#define NVME_CC_EN          (1U << 0)   /* Enable */
#define NVME_CC_CSS_NVM     (0U << 4)   /* NVM Command Set */
#define NVME_CC_MPS_4K      (0U << 7)   /* Host Memory Page Size 4KB (2^(12+0)) */
#define NVME_CC_AMS_RR      (0U << 11)  /* Round Robin Arbitration */
#define NVME_CC_SHN_NONE    (0U << 14)  /* No shutdown notification */
#define NVME_CC_IOSQES_64B  (6U << 16)  /* 2^6 = 64 bytes I/O SQE */
#define NVME_CC_IOCQES_16B  (4U << 20)  /* 2^4 = 16 bytes I/O CQE */

/* CSTS (Controller Status) Bitfields */
#define NVME_CSTS_RDY       (1U << 0)   /* Ready */
#define NVME_CSTS_CFS       (1U << 1)   /* Controller Fatal Status */
#define NVME_CSTS_SHST_MASK (3U << 2)   /* Shutdown Status */

/* Admin Queue Commands (Opcodes) */
#define NVME_ADMIN_OP_DELETE_IO_SQ  0x00
#define NVME_ADMIN_OP_CREATE_IO_SQ  0x01
#define NVME_ADMIN_OP_DELETE_IO_CQ  0x04
#define NVME_ADMIN_OP_CREATE_IO_CQ  0x05
#define NVME_ADMIN_OP_IDENTIFY      0x06
#define NVME_ADMIN_OP_ABORT         0x08
#define NVME_ADMIN_OP_SET_FEATURES  0x09
#define NVME_ADMIN_OP_GET_FEATURES  0x0A

/* I/O Commands (NVM Command Set) */
#define NVME_IO_OP_FLUSH            0x00
#define NVME_IO_OP_WRITE            0x01
#define NVME_IO_OP_READ             0x02

/* Identify CNS Values */
#define NVME_IDENTIFY_CNS_NAMESPACE 0x00
#define NVME_IDENTIFY_CNS_CTRL      0x01

/* NVMe Submission Queue Entry (64 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t  opcode;
    uint8_t  flags;
    uint16_t command_id;
    uint32_t nsid;
    uint64_t reserved;
    uint64_t mptr;
    uint64_t prp1;
    uint64_t prp2;
    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} nvme_sqe_t;

/* NVMe Completion Queue Entry (16 bytes) */
typedef struct __attribute__((packed)) {
    uint32_t cdw0;
    uint32_t reserved;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t command_id;
    uint16_t status; /* bit 0: Phase Tag, bits 15:1: Status Code */
} nvme_cqe_t;

/* Identify Controller Structure (relevant subset of 4096 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t vid;                   /* 0-1: Vendor ID */
    uint16_t ssvid;                 /* 2-3: Subsystem Vendor ID */
    char     sn[20];                /* 4-23: Serial Number (ASCII) */
    char     mn[40];                /* 24-63: Model Number (ASCII) */
    char     fr[8];                 /* 64-71: Firmware Revision (ASCII) */
    uint8_t  rab;                   /* 72: Recommended Arbitration Burst */
    uint8_t  ieee[3];               /* 73-75: IEEE OUI Identifier */
    uint8_t  cmic;                  /* 76: Controller Multi-Path I/O */
    uint8_t  mdts;                  /* 77: Maximum Data Transfer Size */
    uint16_t cntlid;                /* 78-79: Controller ID */
    uint32_t ver;                   /* 80-83: Version */
    uint8_t  rsvd84[432];           /* 84-515: Reserved */
    uint32_t nn;                    /* 516-519: Number of Namespaces */
    uint8_t  rsvd520[3576];         /* 520-4095: Reserved */
} nvme_id_ctrl_t;

/* LBA Format Structure (4 bytes) */
typedef struct __attribute__((packed)) {
    uint16_t ms;                    /* Metadata Size */
    uint8_t  lbads;                 /* LBA Data Size (2^lbads bytes) */
    uint8_t  rp;                    /* Relative Performance */
} nvme_lbaf_t;

/* Identify Namespace Structure (relevant subset of 4096 bytes) */
typedef struct __attribute__((packed)) {
    uint64_t nsze;                  /* 0-7: Namespace Size (Total LBAs) */
    uint64_t ncap;                  /* 8-15: Namespace Capacity (Total allocatable LBAs) */
    uint64_t nuse;                  /* 16-23: Namespace Utilization */
    uint8_t  nsfeat;                /* 24: Namespace Features */
    uint8_t  nlbaf;                 /* 25: Number of LBA Formats */
    uint8_t  flbas;                 /* 26: Formatted LBA Size (bits 3:0 index) */
    uint8_t  mc;                    /* 27: Metadata Capabilities */
    uint8_t  dpc;                   /* 28: End-to-end Data Protection Capabilities */
    uint8_t  dps;                   /* 29: End-to-end Data Protection Type Settings */
    uint8_t  nmic;                  /* 30: Namespace Multi-path I/O */
    uint8_t  rescap;                /* 31: Reservation Capabilities */
    uint8_t  fpi;                   /* 32: Format Progress Indicator */
    uint8_t  dlfeat;                /* 33: Deallocate Logical Block Features */
    uint8_t  rsvd34[94];            /* 34-127: Reserved */
    nvme_lbaf_t lbaf[16];           /* 128-191: LBA Format list */
    uint8_t  rsvd192[3904];         /* 192-4095: Reserved */
} nvme_id_ns_t;

/* Telemetry Record for Forensic Inspection */
typedef struct {
    bool     controller_detected;
    uint8_t  pci_bus;
    uint8_t  pci_slot;
    uint8_t  pci_func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint64_t bar0_phys;
    uint32_t version;
    uint64_t cap;
    uint32_t cc;
    uint32_t csts;
    char     model[41];
    char     serial[21];
    char     firmware[9];
    uint32_t namespace_count;
    uint32_t active_nsid;
    uint64_t sector_count;
    uint32_t sector_size;
    uint64_t capacity_mb;
    int      bdev_id;
    bool     io_queues_created;
} NVMeControllerTelemetry;

/* Driver Lifecycle & I/O APIs */
bool nvme_init(void);
bool nvme_read_sectors(uint32_t nsid, uint64_t lba, uint32_t count, void* buffer);
bool nvme_write_sectors(uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer);
bool nvme_flush(uint32_t nsid);
const NVMeControllerTelemetry* nvme_get_telemetry(void);

#endif /* ATOMS_NVME_H */
