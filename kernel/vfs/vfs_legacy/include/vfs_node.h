#ifndef VFS_NODE_H
#define VFS_NODE_H

#include <stdint.h>
#include "kernel/core/lib/include/list.h"

struct FilesystemDriver; // Forward declaration

typedef enum {
    VFS_FILE,
    VFS_DIRECTORY,
    VFS_MOUNTPOINT
} VFS_NodeType;

typedef struct VFS_Node {
    char name[64];
    VFS_NodeType type;
    uint32_t size;
    
    // Ownership and context
    struct FilesystemDriver* fs_driver;
    void* private_data; // e.g. pointer to FAT32 internal node structure
    
    // Tree hierarchy
    struct VFS_Node* parent;
    
    // Using existing intrusive list for children/siblings
    list_t children;
    list_node_t sibling_node;
} VFS_Node;

#endif // VFS_NODE_H
