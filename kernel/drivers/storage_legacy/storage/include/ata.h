#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

// Standard ATA PIO Ports (Primary Bus)
#define ATA_PRIMARY_IO_BASE    0x1F0
#define ATA_PRIMARY_CTRL_BASE  0x3F6

// Port Offsets from IO Base
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01 // Read
#define ATA_REG_FEATURES   0x01 // Write
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07 // Write
#define ATA_REG_STATUS     0x07 // Read

// Status Register Bits
#define ATA_SR_BSY         0x80    // Busy
#define ATA_SR_DRDY        0x40    // Drive Ready
#define ATA_SR_DF          0x20    // Drive Write Fault
#define ATA_SR_DSC         0x10    // Drive Seek Complete
#define ATA_SR_DRQ         0x08    // Data Request Ready
#define ATA_SR_CORR        0x04    // Corrected Data
#define ATA_SR_IDX         0x02    // Index
#define ATA_SR_ERR         0x01    // Error

// Commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_IDENTIFY          0xEC

// Private data for ATA Block Device
typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
    bool is_master;
    bool supports_lba48;
} ATAPrivateData;

void ata_init(void);

void ata_self_test(void);

#endif // ATA_H
