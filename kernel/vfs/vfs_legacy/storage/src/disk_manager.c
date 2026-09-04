#include "kernel/vfs/vfs_legacy/storage/include/disk_manager.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/vfs/vfs_legacy/storage/include/mbr.h"
#include "kernel/drivers/storage_legacy/storage/include/ata.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"

static LogicalDriveData logical_drives[MAX_LOGICAL_DRIVES];
static BlockDevice logical_block_devices[MAX_LOGICAL_DRIVES];
static int logical_drive_count = 0;

static char* strcpy_custom(char* dest, const char* src) {
    char* orig = dest;
    while ((*dest++ = *src++));
    return orig;
}

// Wrapper read function for logical partitions
static bool logical_partition_read(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    LogicalDriveData* logical_data = (LogicalDriveData*)dev->driver_data;
    
    // Ensure we don't read past the partition boundary
    if (lba + count > logical_data->sector_count) {
        display_print("[DSK] Read out of bounds on logical partition\n");
        return false;
    }

    // Offset LBA by the partition's start LBA
    uint64_t absolute_lba = logical_data->start_lba + lba;
    
    return block_device_read(logical_data->parent_device_id, absolute_lba, count, buffer);
}

// Wrapper write function for logical partitions
static bool logical_partition_write(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    LogicalDriveData* logical_data = (LogicalDriveData*)dev->driver_data;
    
    if (lba + count > logical_data->sector_count) {
        display_print("[DSK] Write out of bounds on logical partition\n");
        return false;
    }

    uint64_t absolute_lba = logical_data->start_lba + lba;
    
    return block_device_write(logical_data->parent_device_id, absolute_lba, count, buffer);
}

static bool logical_partition_flush(BlockDevice* dev) {
    LogicalDriveData* logical_data = (LogicalDriveData*)dev->driver_data;
    return block_device_flush(logical_data->parent_device_id);
}

int disk_manager_register_partition(int parent_device_id, uint64_t start_lba, uint64_t sector_count, uint8_t type) {
    if (logical_drive_count >= MAX_LOGICAL_DRIVES) {
        display_print("[DSK] Max logical drives reached!\n");
        return -1;
    }

    int id = logical_drive_count++;
    
    logical_drives[id].parent_device_id = parent_device_id;
    logical_drives[id].start_lba = start_lba;
    logical_drives[id].sector_count = sector_count;
    logical_drives[id].partition_type = type;

    static char names[MAX_LOGICAL_DRIVES][16];
    static int partition_indices[16];
    int p_num = ++partition_indices[parent_device_id];

    char* name_buf = names[id];
    name_buf[0] = 'd'; name_buf[1] = 'i'; name_buf[2] = 's'; name_buf[3] = 'k';
    name_buf[4] = '0' + (parent_device_id % 10);
    name_buf[5] = 'p';
    name_buf[6] = '0' + (p_num % 10);
    name_buf[7] = '\0';

    logical_block_devices[id].name = name_buf;
    logical_block_devices[id].sector_size = 512;
    logical_block_devices[id].sector_count = sector_count;
    logical_block_devices[id].read_only = false;
    logical_block_devices[id].driver_data = &logical_drives[id];
    logical_block_devices[id].read = logical_partition_read;
    logical_block_devices[id].write = logical_partition_write;
    logical_block_devices[id].flush = logical_partition_flush;

    int new_bd_id = block_device_register(&logical_block_devices[id]);
    
    display_print("[DSK] Partition mapped to ");
    display_print(name_buf);
    display_print(" (Global Block ID: ");
    display_print_dec(new_bd_id);
    display_print(")\n");
    
    return new_bd_id;
}

void disk_manager_init(void) {
    display_print("\n[DSK] Initializing Disk Manager...\n");

    // 1. Initialize core block device registry
    block_device_init();

    // 2. Initialize hardware drivers (Legacy ATA + Native AHCI)
    ata_init();
    extern bool ahci_init(void);
    ahci_init();

    // 3. Scan all registered block devices for partitions
    int initial_devices = block_device_count();
    for (int i = 0; i < initial_devices; i++) {
        // Parse MBR on each physical drive.
        // The MBR parser will call disk_manager_register_partition() for each valid partition it finds.
        mbr_parse(i);
    }
}

int disk_manager_get_logical_drive_count(void) {
    return logical_drive_count;
}

LogicalDriveData* disk_manager_get_logical_drive(int index) {
    if (index < 0 || index >= logical_drive_count) return NULL;
    return &logical_drives[index];
}

BlockDevice* disk_manager_get_logical_block_device(int index) {
    if (index < 0 || index >= logical_drive_count) return NULL;
    return &logical_block_devices[index];
}
