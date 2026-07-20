#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/list.h"
#include "kernel/core/lib/include/string.h"

#define VFS_MAX_MOUNTS 32

static list_t filesystem_registry;
static list_t mount_table;
static int mount_count = 0;
static VFS_Node* vfs_root = NULL;

#include "kernel/core/vizier/include/vizier.h"

void vfs_init(void) {
    VizierContract c = {0};
    c.subsystem_name = "STORAGE";
    c.subsystem_id = VIZIER_SUBSYSTEM_STORAGE;
    vizier_register_subsystem(&c);
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
        } else if (path[0] != '/' && mlen == 1 && mount->mount_path[0] == '/') {
            // Implicit root match for relative paths
            if (1 > best_match_len) {
                best_match = mount;
                best_match_len = 1;
            }
        }
        current = current->next;
    }
    return best_match;
}

// ---------------------------------------------------------
// File API
// ---------------------------------------------------------

#define MAX_OPEN_FILES 32

typedef struct {
    bool in_use;
    VFS_Node* node;
    uint64_t offset;
} VFS_FileDescriptor;

static VFS_FileDescriptor g_fd_table[MAX_OPEN_FILES];

int vfs_open(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount) {
        display_print("[VFS] Open Error: No mount point found for path\n");
        return -1;
    }
    
    int fd = -1;
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (!g_fd_table[i].in_use) {
            fd = i;
            break;
        }
    }
    if (fd < 0) {
        display_print("[VFS] Open Error: FD table full\n");
        return -1;
    }

    // Allocate unique VFS_Node for this open instance so each FD has its own handle & cursor cache
    VFS_Node* file_node = (VFS_Node*)kmalloc(sizeof(VFS_Node));
    if (!file_node) {
        return -1;
    }
    for (uint32_t k = 0; k < sizeof(VFS_Node); k++) ((uint8_t*)file_node)[k] = 0;
    
    int p_idx = 0;
    while (path[p_idx] && p_idx < 63) { file_node->name[p_idx] = path[p_idx]; p_idx++; }
    file_node->name[p_idx] = '\0';
    file_node->type = VFS_FILE;
    file_node->fs_driver = mount->fs_driver;
    file_node->parent = mount->root_node;
    file_node->private_data = mount->root_node->private_data;

    int res = mount->fs_driver->open(file_node, path);
    if (res == 0) {
        g_fd_table[fd].in_use = true;
        g_fd_table[fd].node = file_node;
        g_fd_table[fd].offset = 0;
        return fd;
    }
    kfree(file_node);
    return -1;
}

uint64_t g_last_vfs_read_us = 0;
uint64_t g_max_vfs_read_us = 0;
uint32_t g_last_read_requested = 0;
uint32_t g_last_read_returned = 0;

extern uint64_t step14_rdtsc(void);
extern uint64_t step14_cycles_to_us(uint64_t);

int vfs_read(int fd, void* buffer, uint32_t size) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) {
        display_print("[VFS] Read Error: Invalid FD\n");
        return -1;
    }
    VFS_Node* node = g_fd_table[fd].node;
    if (!node || !node->fs_driver || !node->fs_driver->read) return -1;
    
    g_last_read_requested = size;
    uint64_t start_cycles = step14_rdtsc();

    int res = node->fs_driver->read(node, g_fd_table[fd].offset, size, buffer);
    
    uint64_t end_cycles = step14_rdtsc();
    uint64_t latency = step14_cycles_to_us(end_cycles - start_cycles);
    g_last_vfs_read_us = latency;
    if (latency > g_max_vfs_read_us) {
        g_max_vfs_read_us = latency;
    }
    g_last_read_returned = (res > 0) ? res : 0;

    if (res > 0) {
        g_fd_table[fd].offset += res;
    }
    return res;
}

int vfs_write(int fd, void* buffer, uint32_t size) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) {
        display_print("[VFS] Write Error: Invalid FD\n");
        return -1;
    }
    VFS_Node* node = g_fd_table[fd].node;
    if (!node || !node->fs_driver || !node->fs_driver->write) return -1;
    
    int res = node->fs_driver->write(node, g_fd_table[fd].offset, size, buffer);
    if (res > 0) {
        g_fd_table[fd].offset += res;
    }
    return res;
}

int vfs_pread(int fd, void* buffer, uint32_t size, uint64_t offset) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) {
        display_print("[VFS] PRead Error: Invalid FD\n");
        return -1;
    }
    VFS_Node* node = g_fd_table[fd].node;
    if (!node || !node->fs_driver || !node->fs_driver->read) return -1;
    
    return node->fs_driver->read(node, offset, size, buffer);
}

int vfs_seek(int fd, uint64_t offset, int whence) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) {
        return -1;
    }
    if (whence == 0) { // SEEK_SET
        g_fd_table[fd].offset = offset;
    } else if (whence == 1) { // SEEK_CUR
        g_fd_table[fd].offset += offset;
    } else if (whence == 2) { // SEEK_END
        VFS_Node* node = g_fd_table[fd].node;
        if (node) {
            g_fd_table[fd].offset = node->size + offset;
        } else {
            g_fd_table[fd].offset = offset;
        }
    }
    return g_fd_table[fd].offset;
}

int vfs_close(int fd) {
    if (fd < 3 || fd >= MAX_OPEN_FILES || !g_fd_table[fd].in_use) {
        display_print("[VFS] Close Error: Invalid FD\n");
        return -1;
    }
    VFS_Node* node = g_fd_table[fd].node;
    int res = 0;
    if (node && node->fs_driver && node->fs_driver->close) {
        res = node->fs_driver->close(node);
    }
    if (node) {
        kfree(node);
    }
    g_fd_table[fd].in_use = false;
    g_fd_table[fd].node = NULL;
    g_fd_table[fd].offset = 0;
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
