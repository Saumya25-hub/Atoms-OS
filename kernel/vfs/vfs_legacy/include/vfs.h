#ifndef VFS_H
#define VFS_H

#include "kernel/vfs/vfs_legacy/include/vfs_node.h"
#include "kernel/vfs/vfs_legacy/include/vfs_mount.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

// Structure for directory entries returned by readdir
typedef struct {
    char name[64];
    uint32_t size;
    uint8_t is_directory;
    uint32_t cluster;
} vfs_dirent_t;

// Standard driver interface that all filesystems must implement
typedef struct FilesystemDriver {
    const char* name;
    
    // Core driver operations
    VFS_Node* (*mount)(BlockDevice* device);
    int       (*unmount)(VFS_Node* root_node);
    int       (*open)(VFS_Node* node, const char* path);
    int       (*read)(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer);
    int       (*write)(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer);
    int       (*close)(VFS_Node* node);
    int       (*readdir)(VFS_Node* node, const char* path, int index, vfs_dirent_t* out_entry);
    int       (*mkdir)(VFS_Node* node, const char* name);
    int       (*create)(VFS_Node* node, const char* name);
    int       (*rename)(VFS_Node* node, const char* old_path, const char* new_name);
    int       (*delete)(VFS_Node* node, const char* path);
    
    // Kept in a registry
    list_node_t list_node;
} FilesystemDriver;

// VFS System Initialization
void vfs_init(void);

// Filesystem Registration API (e.g. fat32_init calls this)
int vfs_register_fs(FilesystemDriver* driver);

// Mount Manager API
int vfs_mount_fs(const char* path, int block_device_id, const char* fs_name);
int vfs_unmount_fs(const char* path);
VFS_Mount* vfs_get_mount(const char* path);

// Filesystem Auto-Detection API
const char* vfs_detect_fs(BlockDevice* device);

// High-level syscall stubs mapped to VFS backend
int vfs_open(const char* path);
int vfs_read(int fd, void* buffer, uint32_t size);
int vfs_write(int fd, void* buffer, uint32_t size);
int vfs_pread(int fd, void* buffer, uint32_t size, uint64_t offset);
int vfs_seek(int fd, uint64_t offset, int whence);
int vfs_close(int fd);
int vfs_readdir(const char* path, int index, vfs_dirent_t* out_entry);
int vfs_mkdir(const char* path);
int vfs_create(const char* path);
int vfs_rename(const char* old_path, const char* new_name);
int vfs_delete(const char* path);

void vfs_self_test(void);

#endif // VFS_H
