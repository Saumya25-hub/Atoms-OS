#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_BLOCK_DEVICES 16

// Generic Partition Structure for future MBR/GPT parsing
typedef struct {
    uint64_t start_lba;
    uint64_t sector_count;
    uint8_t  type;
    bool     bootable;
} Partition;

// Hardware-agnostic Block Device Structure
typedef struct BlockDevice {
    int id;
    const char* name;
    uint64_t sector_size;
    uint64_t sector_count;
    bool read_only;
    
    // Hardware-private context (e.g., ATA port base, NVMe queue ptr)
    void* driver_data;

    // Callbacks to hardware-specific functions
    bool (*read)(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer);
    bool (*write)(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer);
    bool (*flush)(struct BlockDevice* dev);
} BlockDevice;

// Block Device Registry API
void block_device_init(void);
int block_device_register(BlockDevice* device);
BlockDevice* block_device_get(int index);
int block_device_count(void);

// Block Device I/O API (Always use these, never call hardware directly)
bool block_device_read(int index, uint64_t lba, uint32_t count, void* buffer);
bool block_device_write(int index, uint64_t lba, uint32_t count, void* buffer);
bool block_device_flush(int index);

#endif // BLOCK_DEVICE_H
