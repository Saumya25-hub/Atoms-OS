#include "../include/usb_mount.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern bool usb_sector_cache_read(usb_disk_t* disk, uint32_t lba, uint8_t* buffer);

typedef struct {
    usb_volume_t* vol;
    uint32_t      position;
    bool          is_open;
} usb_vfs_file_t;

usb_vfs_file_t* usb_vfs_open(const char* path) {
    if (!path) return NULL;
    static usb_vfs_file_t handle;
    memset(&handle, 0, sizeof(handle));
    handle.vol = usb_volume_get_by_letter(path[0]);
    handle.is_open = true;
    return &handle;
}

uint32_t usb_vfs_read(usb_vfs_file_t* file, uint8_t* buffer, uint32_t bytes) {
    if (!file || !file->is_open || !file->vol || !buffer) return 0;
    
    usb_disk_t* disk = file->vol->partition ? file->vol->partition->disk : NULL;
    if (!disk) return 0;
    
    uint32_t sectors = (bytes + 511) / 512;
    for (uint32_t i = 0; i < sectors; i++) {
        usb_sector_cache_read(disk, i, buffer + (i * 512));
    }
    file->position += bytes;
    return bytes;
}

uint32_t usb_vfs_write(usb_vfs_file_t* file, const uint8_t* buffer, uint32_t bytes) {
    if (!file || !file->is_open || !file->vol || !buffer) return 0;
    
    usb_disk_t* disk = file->vol->partition ? file->vol->partition->disk : NULL;
    if (!disk) return 0;
    
    uint32_t sectors = (bytes + 511) / 512;
    usb_disk_write_sectors(disk, 0, sectors, buffer);
    file->position += bytes;
    return bytes;
}

void usb_vfs_close(usb_vfs_file_t* file) {
    if (file) file->is_open = false;
}
