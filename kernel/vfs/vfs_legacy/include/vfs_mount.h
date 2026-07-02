#ifndef VFS_MOUNT_H
#define VFS_MOUNT_H

#include "kernel/vfs/vfs_legacy/include/vfs_node.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

struct FilesystemDriver; // Forward declaration

typedef struct VFS_Mount {
    char mount_path[128];               // e.g. "/", "/mnt/usb"
    BlockDevice* block_device;          // The underlying device mapped here
    struct FilesystemDriver* fs_driver; // The driver handling this mount
    VFS_Node* root_node;                // The root node of this mounted filesystem
    
    // Mount points are kept in a linked list for easy searching
    list_node_t list_node;
} VFS_Mount;

#endif // VFS_MOUNT_H
