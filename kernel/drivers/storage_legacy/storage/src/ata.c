#include "kernel/drivers/storage_legacy/storage/include/ata.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include <stddef.h>

extern void io_out8(uint16_t port, uint8_t data);
extern uint8_t io_in8(uint16_t port);
extern void io_out16(uint16_t port, uint16_t data);
extern uint16_t io_in16(uint16_t port);

#define MAX_ATA_DRIVES 4
static ATAPrivateData ata_drives_data[MAX_ATA_DRIVES];
static BlockDevice ata_block_devices[MAX_ATA_DRIVES];
static int ata_drive_count = 0;

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

static volatile bool ata_lock = false;

static inline void ata_insw(uint16_t port, void* addr, uint32_t count) {
    __asm__ volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static inline void ata_outsw(uint16_t port, const void* addr, uint32_t count) {
    __asm__ volatile("rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

uint32_t g_ata_read_count = 0;

static bool ata_read_sectors_internal(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !buffer || count == 0) return false;
    if (lba + count > dev->sector_count) return false;

    g_ata_read_count++;

    while (__sync_lock_test_and_set(&ata_lock, 1)) {
        // Spin until unlocked to prevent reentrancy from multitasking
    }

    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    uint16_t io_base = priv->io_base;
    uint8_t* ptr = (uint8_t*)buffer;

    // For simplicity, we only implement 28-bit LBA in Sprint 2.
    // Ensure LBA fits in 28 bits.
    if (lba >= 0x10000000) {
        display_print("[ATA] Error: LBA out of 28-bit range\n");
        __sync_lock_release(&ata_lock);
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

        uint64_t flags;
        __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags));
        ata_insw(io_base + ATA_REG_DATA, ptr, 256);
        if (flags & 0x200) __asm__ volatile("sti");
        ptr += 512;
    }

    __sync_lock_release(&ata_lock);
    return true;
}

static bool ata_write_sectors_internal(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    while (__sync_lock_test_and_set(&ata_lock, 1)) {
        // Spin until unlocked
    }

    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    uint16_t io_base = priv->io_base;
    uint8_t* ptr = (uint8_t*)buffer;

    if (lba >= 0x10000000) {
        display_print("[ATA] Error: LBA out of 28-bit range\n");
        __sync_lock_release(&ata_lock);
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

        uint64_t flags;
        __asm__ volatile("pushfq; popq %0; cli" : "=r"(flags));
        ata_outsw(io_base + ATA_REG_DATA, ptr, 256);
        if (flags & 0x200) __asm__ volatile("sti");
        ptr += 512;
    }
    
    // Cache flush (E7h) is good practice after writes
    io_out8(io_base + ATA_REG_COMMAND, 0xE7);
    ata_wait_bsy(io_base);

    __sync_lock_release(&ata_lock);
    return true;
}

static bool ata_flush_internal(BlockDevice* dev) {
    ATAPrivateData* priv = (ATAPrivateData*)dev->driver_data;
    io_out8(priv->io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy(priv->io_base);
    return true;
}

static bool ata_probe_single_drive(uint16_t io_base, uint16_t ctrl_base, bool is_master, const char* name) {
    if (ata_drive_count >= MAX_ATA_DRIVES) return false;

    uint8_t dev_sel = is_master ? 0xA0 : 0xB0;
    io_out8(io_base + ATA_REG_HDDEVSEL, dev_sel);
    ata_delay(io_base);

    io_out8(io_base + ATA_REG_SECCOUNT0, 0);
    io_out8(io_base + ATA_REG_LBA0, 0);
    io_out8(io_base + ATA_REG_LBA1, 0);
    io_out8(io_base + ATA_REG_LBA2, 0);

    io_out8(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay(io_base);

    uint8_t status = io_in8(io_base + ATA_REG_STATUS);
    if (status == 0 || status == 0xFF) return false;

    int timeout = 10000;
    while (timeout > 0) {
        status = io_in8(io_base + ATA_REG_STATUS);
        if (status & ATA_SR_ERR) return false;
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
        timeout--;
        for (volatile int delay = 0; delay < 100; delay++) {}
    }
    if (timeout <= 0) return false;

    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = io_in16(io_base + ATA_REG_DATA);
    }

    uint32_t total_sectors = (uint32_t)identify_data[60] | ((uint32_t)identify_data[61] << 16);
    if (total_sectors == 0) return false;

    int idx = ata_drive_count++;
    ATAPrivateData* priv = &ata_drives_data[idx];
    BlockDevice* dev = &ata_block_devices[idx];

    priv->io_base = io_base;
    priv->ctrl_base = ctrl_base;
    priv->is_master = is_master;
    priv->supports_lba48 = false;

    dev->name = name;
    dev->sector_size = 512;
    dev->sector_count = total_sectors;
    dev->read_only = false;
    dev->driver_data = priv;
    dev->read = ata_read_sectors_internal;
    dev->write = ata_write_sectors_internal;
    dev->flush = ata_flush_internal;

    int bd_id = block_device_register(dev);
    display_print("[ATA] Discovered "); display_print(name);
    display_print("! Sectors: "); display_print_dec(total_sectors);
    display_print(" (Global Block ID: "); display_print_dec(bd_id); display_print(")\n");
    return true;
}

void ata_init(void) {
    display_print("[ATA] Probing Physical ATA Storage Controller Drives...\n");
    ata_drive_count = 0;

    ata_probe_single_drive(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, true, "ATA_PM");
    ata_probe_single_drive(ATA_PRIMARY_IO_BASE, ATA_PRIMARY_CTRL_BASE, false, "ATA_PS");
    ata_probe_single_drive(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, true, "ATA_SM");
    ata_probe_single_drive(ATA_SECONDARY_IO_BASE, ATA_SECONDARY_CTRL_BASE, false, "ATA_SS");
}

void ata_self_test(void) {
    if (ata_drive_count == 0) {
        display_print("[SELF TEST] ATA: FAILED (No drive)\n");
    } else {
        display_print("[SELF TEST] ATA: PASS\n");
    }
}
