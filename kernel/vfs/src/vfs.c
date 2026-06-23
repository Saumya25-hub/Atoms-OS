#include "kernel/vfs/include/vfs.h"
#include "kernel/display/display.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/list.h"
#include "kernel/lib/include/string.h"

#define VFS_MAX_MOUNTS 32

static list_t filesystem_registry;
static list_t mount_table;
static int mount_count = 0;
static VFS_Node* vfs_root = NULL;

void vfs_init(void) {
    display_print("\n[VFS] Initializing...\n");
    
    list_init(&filesystem_registry);
    display_print("[VFS] Filesystem Registry Initialized\n");
    
    list_init(&mount_table);
    display_print("[VFS] Mount Manager Initialized\n");
    
    // In Sprint 2, we will create the global VFS root node and handle actual mounts.
    vfs_root = NULL;
    
    display_print("[VFS] Ready\n");
}

int vfs_register_fs(FilesystemDriver* driver) {
    if (!driver) return -1;
    
    list_node_init(&driver->list_node);
    list_insert_tail(&filesystem_registry, &driver->list_node);
    
    display_print("[VFS] Registered filesystem driver: ");
    display_print(driver->name);
    display_print("\n");
    
    return 0;
}

// ---------------------------------------------------------
// Mount Manager API
// ---------------------------------------------------------

static FilesystemDriver* vfs_find_fs_driver(const char* name) {
    list_node_t* current = filesystem_registry.head;
    while (current) {
        FilesystemDriver* driver = LIST_ENTRY(current, FilesystemDriver, list_node);
        if (strcmp(driver->name, name) == 0) {
            return driver;
        }
        current = current->next;
    }
    return NULL;
}

int vfs_mount_fs(const char* path, int block_device_id, const char* fs_name) {
    display_print("[VFS] Attempting to mount Block Device ");
    display_print_dec(block_device_id);
    display_print(" to ");
    display_print(path);
    display_print(" using ");
    display_print(fs_name);
    display_print("\n");

    BlockDevice* bdev = block_device_get(block_device_id);
    if (!bdev) {
        display_print("[VFS] Mount Error: Invalid block device ID\n");
        return -1;
    }

    FilesystemDriver* fs_driver = vfs_find_fs_driver(fs_name);
    if (!fs_driver) {
        display_print("[VFS] Mount Error: Filesystem driver not found\n");
        return -2;
    }

    // Rule 123: Mount Table Limit
    if (mount_count >= VFS_MAX_MOUNTS) {
        display_print("[VFS] Mount Error: VFS_MAX_MOUNTS limit reached\n");
        return -3;
    }

    // Rule 122: Duplicate Mount Protection
    if (vfs_get_mount(path)) {
        display_print("[VFS] Mount Error: Path already mounted\n");
        return -4;
    }

    VFS_Node* root_node = fs_driver->mount(bdev);
    if (!root_node) {
        display_print("[VFS] Mount Error: Filesystem driver rejected mount\n");
        return -5;
    }

    VFS_Mount* new_mount = (VFS_Mount*)kmalloc(sizeof(VFS_Mount));
    if (!new_mount) {
        display_print("[VFS] Mount Error: Out of memory\n");
        return -6;
    }

    strcpy(new_mount->mount_path, path);
    new_mount->block_device = bdev;
    new_mount->fs_driver = fs_driver;
    new_mount->root_node = root_node;

    list_node_init(&new_mount->list_node);
    list_insert_tail(&mount_table, &new_mount->list_node);
    mount_count++;

    display_print("[VFS] Mount SUCCESS!\n");
    return 0;
}

VFS_Mount* vfs_get_mount(const char* path) {
    VFS_Mount* best_match = NULL;
    size_t best_match_len = 0;

    list_node_t* current = mount_table.head;
    while (current) {
        VFS_Mount* mount = LIST_ENTRY(current, VFS_Mount, list_node);
        size_t mlen = strlen(mount->mount_path);
        
        // Basic prefix match
        if (strncmp(path, mount->mount_path, mlen) == 0) {
            if (mlen > best_match_len) {
                best_match = mount;
                best_match_len = mlen;
            }
        }
        current = current->next;
    }
    return best_match;
}

// ---------------------------------------------------------
// File API
// ---------------------------------------------------------

// For Sprint 3, we mock file descriptors since we don't have a FD table yet.
static VFS_Node* mock_fd_node = NULL;
static uint64_t mock_fd_offset = 0;

int vfs_open(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount) {
        display_print("[VFS] Open Error: No mount point found for path\n");
        return -1;
    }
    
    // We pass the root node to the driver's open function.
    int res = mount->fs_driver->open(mount->root_node, path);
    if (res == 0) {
        // Mock successful FD
        mock_fd_node = mount->root_node;
        mock_fd_offset = 0;
        return 3; // return a fake FD
    }
    return -1;
}

int vfs_read(int fd, void* buffer, uint32_t size) {
    if (fd != 3 || !mock_fd_node) {
        display_print("[VFS] Read Error: Invalid FD\n");
        return -1;
    }
    
    int res = mock_fd_node->fs_driver->read(mock_fd_node, mock_fd_offset, size, buffer);
    if (res > 0) {
        mock_fd_offset += res;
    }
    return res;
}

int vfs_write(int fd, void* buffer, uint32_t size) {
    if (fd != 3 || !mock_fd_node) {
        display_print("[VFS] Write Error: Invalid FD\n");
        return -1;
    }
    if (!mock_fd_node->fs_driver->write) return -1;
    
    int res = mock_fd_node->fs_driver->write(mock_fd_node, mock_fd_offset, size, buffer);
    if (res > 0) {
        mock_fd_offset += res;
    }
    return res;
}

int vfs_pread(int fd, void* buffer, uint32_t size, uint64_t offset) {
    if (fd != 3 || !mock_fd_node) {
        display_print("[VFS] PRead Error: Invalid FD\n");
        return -1;
    }
    
    return mock_fd_node->fs_driver->read(mock_fd_node, offset, size, buffer);
}

int vfs_close(int fd) {
    if (fd != 3 || !mock_fd_node) {
        display_print("[VFS] Close Error: Invalid FD\n");
        return -1;
    }
    
    int res = mock_fd_node->fs_driver->close(mock_fd_node);
    mock_fd_node = NULL;
    mock_fd_offset = 0;
    return res;
}

int vfs_readdir(const char* path, int index, vfs_dirent_t* out_entry) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount) return -1;
    if (!mount->fs_driver->readdir) return -1;

    return mount->fs_driver->readdir(mount->root_node, path, index, out_entry);
}

int vfs_mkdir(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->mkdir) return -1;
    return mount->fs_driver->mkdir(mount->root_node, path);
}

int vfs_create(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->create) return -1;
    return mount->fs_driver->create(mount->root_node, path);
}

int vfs_rename(const char* old_path, const char* new_name) {
    VFS_Mount* mount = vfs_get_mount(old_path);
    if (!mount || !mount->fs_driver->rename) return -1;
    return mount->fs_driver->rename(mount->root_node, old_path, new_name);
}

int vfs_delete(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->delete) return -1;
    return mount->fs_driver->delete(mount->root_node, path);
}

void vfs_self_test(void) {
    if (vfs_get_mount("/") != NULL) {
        display_print("[SELF TEST] VFS: PASS (Root Mounted)\n");
    } else {
        display_print("[SELF TEST] VFS: FAILED (No Root Mount)\n");
    }
}
