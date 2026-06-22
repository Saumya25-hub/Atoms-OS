#ifndef VFS_H
#define VFS_H

#include "kernel/vfs/include/vfs_node.h"
#include "kernel/vfs/include/vfs_mount.h"
#include "kernel/storage/include/block_device.h"

// Standard driver interface that all filesystems must implement
typedef struct FilesystemDriver {
    const char* name;
    
    // Core driver operations
    VFS_Node* (*mount)(BlockDevice* device);
    int       (*open)(VFS_Node* node, const char* path);
    int       (*read)(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer);
    int       (*close)(VFS_Node* node);
    
    // Kept in a registry
    list_node_t list_node;
} FilesystemDriver;

// VFS System Initialization
void vfs_init(void);

// Filesystem Registration API (e.g. fat32_init calls this)
int vfs_register_fs(FilesystemDriver* driver);

// Mount Manager API
int vfs_mount_fs(const char* path, int block_device_id, const char* fs_name);
VFS_Mount* vfs_get_mount(const char* path);

// High-level syscall stubs mapped to VFS backend
int vfs_open(const char* path);
int vfs_read(int fd, void* buffer, uint32_t size);
int vfs_pread(int fd, void* buffer, uint32_t size, uint64_t offset);
int vfs_close(int fd);

void vfs_self_test(void);

#endif // VFS_H
