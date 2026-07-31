#ifndef BOS_PACK_LOADER_H
#define BOS_PACK_LOADER_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_PACK_MAX_PATH 256
#define BOS_PACK_MAGIC 0x58534F42 // "BOSX"

typedef struct {
    char package_path[BOS_PACK_MAX_PATH];
    uint32_t magic;
    uint32_t file_size;
    bool is_valid;
    bool is_mounted;
    void* zip_handle;
} BOS_Package;

BOS_Package* bos_package_open(const char* path);
bool bos_package_verify(BOS_Package* pack);
bool bos_package_mount(BOS_Package* pack, const char* mount_point);
void bos_package_close(BOS_Package* pack);

#endif /* BOS_PACK_LOADER_H */
