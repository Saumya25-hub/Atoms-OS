#include "kernel/driver/storage/include/ata.h"
#include "kernel/storage/include/block_device.h"
#include "kernel/display/display.h"
#include "kernel/memory/heap/include/heap.h"
#include <stddef.h>

extern void io_out8(uint16_t port, uint8_t data);
extern uint8_t io_in8(uint16_t port);
extern void io_out16(uint16_t port, uint16_t data);
extern uint16_t io_in16(uint16_t port);

static ATAPrivateData primary_master;
static BlockDevice ata_block_device;

static void ata_delay(uint16_t io_base) {
    // 400ns delay: read alternate status register 4 times
    for (int i = 0; i < 4; i++) {
        io_in8(io_base + ATA_REG_STATUS);
    }
}

static void ata_wait_bsy(uint16_t io_base) {
    while (io_in8(io_base + ATA_REG_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(uint16_t io_base) {
    while (!(io_in8(io_base + ATA_REG_STATUS) & ATA_SR_DRQ));
}

static bool ata_read_sectors_internal(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    uint16_t io_base = priv->io_base;
    uint8_t* ptr = (uint8_t*)buffer;

    // For simplicity, we only implement 28-bit LBA in Sprint 2.
    // Ensure LBA fits in 28 bits.
    if (lba >= 0x10000000) {
        display_print("[ATA] Error: LBA out of 28-bit range\n");
        return false;
    }

    ata_wait_bsy(io_base);

    // Select drive and send highest 4 bits of LBA
    // 0xE0 for Master, 0xF0 for Slave
    uint8_t drive_sel = priv->is_master ? 0xE0 : 0xF0;
    io_out8(io_base + ATA_REG_HDDEVSEL, drive_sel | ((lba >> 24) & 0x0F));

    io_out8(io_base + ATA_REG_SECCOUNT0, count);
    io_out8(io_base + ATA_REG_LBA0, (uint8_t)lba);
    io_out8(io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    io_out8(io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    io_out8(io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (uint32_t i = 0; i < count; i++) {
        ata_wait_bsy(io_base);
        ata_wait_drq(io_base);

        for (int j = 0; j < 256; j++) {
            uint16_t word = io_in16(io_base + ATA_REG_DATA);
            ptr[j * 2] = (uint8_t)word;
            ptr[j * 2 + 1] = (uint8_t)(word >> 8);
        }
        ptr += 512;
    }

    return true;
}

static bool ata_write_sectors_internal(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    uint16_t io_base = priv->io_base;
    uint8_t* ptr = (uint8_t*)buffer;

    if (lba >= 0x10000000) {
        display_print("[ATA] Error: LBA out of 28-bit range\n");
        return false;
    }

    ata_wait_bsy(io_base);

    uint8_t drive_sel = priv->is_master ? 0xE0 : 0xF0;
    io_out8(io_base + ATA_REG_HDDEVSEL, drive_sel | ((lba >> 24) & 0x0F));

    io_out8(io_base + ATA_REG_SECCOUNT0, count);
    io_out8(io_base + ATA_REG_LBA0, (uint8_t)lba);
    io_out8(io_base + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    io_out8(io_base + ATA_REG_LBA2, (uint8_t)(lba >> 16));

    io_out8(io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    for (uint32_t i = 0; i < count; i++) {
        ata_wait_bsy(io_base);
        ata_wait_drq(io_base);

        for (int j = 0; j < 256; j++) {
            uint16_t word = ptr[j * 2] | (ptr[j * 2 + 1] << 8);
            io_out16(io_base + ATA_REG_DATA, word);
        }
        ptr += 512;
    }
    
    // Flush cache after write
    io_out8(io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy(io_base);

    return true;
}

static bool ata_flush_internal(BlockDevice* dev) {
    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    io_out8(priv->io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy(priv->io_base);
    return true;
}

void ata_init(void) {
    display_print("[ATA] Initializing Primary Master...\n");

    uint16_t io_base = ATA_PRIMARY_IO_BASE;

    // 1. Select Drive
    io_out8(io_base + ATA_REG_HDDEVSEL, 0xA0); // Master
    
    // 2. Set Sector Count & LBA to 0
    io_out8(io_base + ATA_REG_SECCOUNT0, 0);
    io_out8(io_base + ATA_REG_LBA0, 0);
    io_out8(io_base + ATA_REG_LBA1, 0);
    io_out8(io_base + ATA_REG_LBA2, 0);

    // 3. Send IDENTIFY Command
    io_out8(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    uint8_t status = io_in8(io_base + ATA_REG_STATUS);
    if (status == 0) {
        display_print("[ATA] Drive not found.\n");
        return;
    }

    // Wait until BSY clears
    int timeout = 100000;
    while (timeout > 0) {
        status = io_in8(io_base + ATA_REG_STATUS);
        if ((status & ATA_SR_ERR)) {
            display_print("[ATA] Error identifying drive (Not ATA?).\n");
            return;
        }
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
            break; // Ready to read
        }
        timeout--;
        for (volatile int delay = 0; delay < 100; delay++) {} // Small delay
    }

    if (timeout <= 0) {
        display_print("[ATA] Timeout waiting for drive to become ready.\n");
        return;
    }

    // Read 256 16-bit words of IDENTIFY data
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = io_in16(io_base + ATA_REG_DATA);
    }

    // Identify data words:
    // Word 60 & 61: Total number of 28-bit LBA addressable sectors
    uint32_t total_sectors = (uint32_t)identify_data[60] | ((uint32_t)identify_data[61] << 16);

    display_print("[ATA] Drive Identified! Sectors: ");
    display_print_dec(total_sectors);
    display_print("\n");

    // Initialize the private data
    primary_master.io_base = io_base;
    primary_master.ctrl_base = ATA_PRIMARY_CTRL_BASE;
    primary_master.is_master = true;
    primary_master.supports_lba48 = false; // Simplified for Sprint 2

    // Initialize BlockDevice struct
    ata_block_device.name = "ATA_PM"; // Primary Master
    ata_block_device.sector_size = 512;
    ata_block_device.sector_count = total_sectors;
    ata_block_device.read_only = false;
    ata_block_device.driver_data = &primary_master;
    ata_block_device.read = ata_read_sectors_internal;
    ata_block_device.write = ata_write_sectors_internal;
    ata_block_device.flush = ata_flush_internal;

    // Register with the Block Device Layer!
    block_device_register(&ata_block_device);
}

void ata_self_test(void) {
    if (ata_block_device.driver_data == NULL) {
        display_print("[SELF TEST] ATA: FAILED (No drive)\n");
    } else {
        display_print("[SELF TEST] ATA: PASS\n");
    }
}
