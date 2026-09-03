#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/list.h"
#include "kernel/core/lib/include/string.h"

#define VFS_MAX_MOUNTS 32
#define MAX_OPEN_FILES 32

typedef struct {
    bool in_use;
    VFS_Node* node;
    uint64_t offset;
} VFS_FileDescriptor;

static list_t filesystem_registry;
static list_t mount_table;
static int mount_count = 0;
static VFS_Node* vfs_root = NULL;
static VFS_FileDescriptor g_fd_table[MAX_OPEN_FILES];

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

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        g_fd_table[i].in_use = false;
        g_fd_table[i].node = NULL;
        g_fd_table[i].offset = 0;
    }
    
    // In Sprint 2, we will create the global VFS root node and handle actual mounts.
    vfs_root = NULL;
    
    display_print("[VFS] Ready\n");
}

int vfs_register_fs(FilesystemDriver* driver) {
    if (!driver) return -1;
    
    // Protection against duplicate registration list cycle
    list_node_t* current = filesystem_registry.head;
    while (current) {
        FilesystemDriver* existing = LIST_ENTRY(current, FilesystemDriver, list_node);
        if (existing == driver || (existing->name && driver->name && strcmp(existing->name, driver->name) == 0)) {
            return 0; // Already registered cleanly
        }
        current = current->next;
    }

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

    // Rule 122: Duplicate Mount Protection (Exact Path Match)
    list_node_t* check_curr = mount_table.head;
    while (check_curr) {
        VFS_Mount* m = LIST_ENTRY(check_curr, VFS_Mount, list_node);
        if (strcmp(m->mount_path, path) == 0) {
            display_print("[VFS] Mount Error: Path already mounted\n");
            return -4;
        }
        check_curr = check_curr->next;
    }

    VFS_Node* root_node = fs_driver->mount(bdev);
    if (!root_node) {
        display_print("[VFS] Mount Error: Filesystem driver rejected mount\n");
        return -5;
    }

    VFS_Mount* new_mount = (VFS_Mount*)kmalloc(sizeof(VFS_Mount));
    if (!new_mount) {
        display_print("[VFS] Mount Error: Out of memory\n");
        if (fs_driver->unmount) {
            fs_driver->unmount(root_node);
        } else {
            if (root_node->private_data) {
                kfree(root_node->private_data);
                root_node->private_data = NULL;
            }
            kfree(root_node);
        }
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

static const char* vfs_strip_mount_prefix(VFS_Mount* mount, const char* path) {
    if (!mount || !path) return path;
    size_t mlen = strlen(mount->mount_path);
    if (mlen == 1 && mount->mount_path[0] == '/') {
        return path;
    }
    if (strncmp(path, mount->mount_path, mlen) == 0) {
        const char* rel = path + mlen;
        if (rel[0] == '\0') return "/";
        return rel;
    }
    return path;
}

int vfs_unmount_fs(const char* path) {
    if (!path) return -1;

    list_node_t* current = mount_table.head;
    while (current) {
        list_node_t* next = current->next;
        VFS_Mount* mount = LIST_ENTRY(current, VFS_Mount, list_node);
        if (strcmp(mount->mount_path, path) == 0) {
            // Check whether mount is busy with active open file descriptors
            for (int i = 0; i < MAX_OPEN_FILES; i++) {
                if (g_fd_table[i].in_use && g_fd_table[i].node) {
                    if (g_fd_table[i].node->parent == mount->root_node ||
                        g_fd_table[i].node == mount->root_node) {
                        display_print("[VFS] Unmount Error: Target filesystem is busy (open files)\n");
                        return -16; // -EBUSY
                    }
                }
            }

            // Remove from mount table
            list_remove(&mount_table, &mount->list_node);
            if (mount_count > 0) mount_count--;

            // Teardown filesystem private state and root node
            int fs_status = 0;
            if (mount->fs_driver && mount->fs_driver->unmount) {
                fs_status = mount->fs_driver->unmount(mount->root_node);
            } else if (mount->root_node) {
                if (mount->root_node->private_data) {
                    kfree(mount->root_node->private_data);
                    mount->root_node->private_data = NULL;
                }
                kfree(mount->root_node);
            }

            // Free the VFS_Mount object itself
            kfree(mount);

            display_print("[VFS] Unmounted cleanly: ");
            display_print(path);
            display_print("\n");

            return (fs_status == 0) ? 0 : -1;
        }
        current = next;
    }

    display_print("[VFS] Unmount Error: Mount path not found: ");
    display_print(path);
    display_print("\n");
    return -1;
}

VFS_Mount* vfs_get_mount(const char* path) {
    VFS_Mount* best_match = NULL;
    size_t best_match_len = 0;

    list_node_t* current = mount_table.head;
    while (current) {
        VFS_Mount* mount = LIST_ENTRY(current, VFS_Mount, list_node);
        size_t mlen = strlen(mount->mount_path);
        
        // Boundary-aware prefix match
        if (strncmp(path, mount->mount_path, mlen) == 0) {
            if (path[mlen] == '\0' || path[mlen] == '/' || mount->mount_path[mlen - 1] == '/') {
                if (mlen > best_match_len) {
                    best_match = mount;
                    best_match_len = mlen;
                }
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

uint32_t vfs_get_mount_count(void) {
    return (uint32_t)mount_count;
}

bool vfs_get_mount_info(uint32_t index, char* out_path, uint32_t max_path, char* out_fs, uint32_t max_fs, char* out_dev, uint32_t max_dev) {
    if (index >= (uint32_t)mount_count) return false;

    uint32_t current_idx = 0;
    list_node_t* node = mount_table.head;
    while (node) {
        VFS_Mount* mount = LIST_ENTRY(node, VFS_Mount, list_node);
        if (current_idx == index && mount) {
            if (out_path && max_path > 0) {
                strncpy(out_path, mount->mount_path, max_path - 1);
                out_path[max_path - 1] = '\0';
            }
            if (out_fs && max_fs > 0) {
                const char* fs_name = (mount->fs_driver && mount->fs_driver->name) ? mount->fs_driver->name : "unknown";
                strncpy(out_fs, fs_name, max_fs - 1);
                out_fs[max_fs - 1] = '\0';
            }
            if (out_dev && max_dev > 0) {
                const char* dev_name = (mount->block_device && mount->block_device->name) ? mount->block_device->name : "none";
                strncpy(out_dev, dev_name, max_dev - 1);
                out_dev[max_dev - 1] = '\0';
            }
            return true;
        }
        current_idx++;
        node = node->next;
    }
    return false;
}

// ---------------------------------------------------------
// Filesystem Auto-Detection API
// ---------------------------------------------------------
const char* vfs_detect_fs(BlockDevice* device) {
    if (!device || device->sector_size == 0) return NULL;

    uint8_t buffer[512];
    if (!block_device_read(device->id, 0, 1, buffer)) return NULL;

    uint16_t boot_sig = *((uint16_t*)(buffer + 510));
    if (boot_sig != 0xAA55) return NULL;

    // Check NTFS OEM Signature ("NTFS    ")
    if (buffer[3] == 'N' && buffer[4] == 'T' && buffer[5] == 'F' && buffer[6] == 'S' &&
        buffer[7] == ' ' && buffer[8] == ' ' && buffer[9] == ' ' && buffer[10] == ' ') {
        uint16_t bps = *((uint16_t*)(buffer + 0x0B));
        uint8_t spc = buffer[0x0D];
        if (bps >= 512 && spc != 0) {
            return "ntfs";
        }
    }

    // Check FAT32 Signatures and Parameters
    uint16_t bps = *((uint16_t*)(buffer + 0x0B));
    uint8_t spc = buffer[0x0D];
    uint16_t res_sec = *((uint16_t*)(buffer + 0x0E));
    uint8_t fat_cnt = buffer[0x10];
    uint32_t fat_sz = *((uint32_t*)(buffer + 0x24));
    uint32_t root_cls = *((uint32_t*)(buffer + 0x2C));
    uint8_t boot_sig_byte = buffer[0x42];

    if (bps == 512 && spc != 0 && res_sec != 0 && fat_cnt == 2 && fat_sz != 0 &&
        root_cls >= 2 && (boot_sig_byte == 0x29 || boot_sig_byte == 0x28)) {
        return "fat32";
    }

    return NULL;
}

// ---------------------------------------------------------
// File API
// ---------------------------------------------------------

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

    const char* rel_path = vfs_strip_mount_prefix(mount, path);
    int res = mount->fs_driver->open(file_node, rel_path);
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

    const char* rel_path = vfs_strip_mount_prefix(mount, path);
    return mount->fs_driver->readdir(mount->root_node, rel_path, index, out_entry);
}

int vfs_mkdir(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->mkdir) return -1;
    return mount->fs_driver->mkdir(mount->root_node, vfs_strip_mount_prefix(mount, path));
}

int vfs_create(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->create) return -1;
    return mount->fs_driver->create(mount->root_node, vfs_strip_mount_prefix(mount, path));
}

int vfs_rename(const char* old_path, const char* new_name) {
    VFS_Mount* mount = vfs_get_mount(old_path);
    if (!mount || !mount->fs_driver->rename) return -1;
    return mount->fs_driver->rename(mount->root_node, vfs_strip_mount_prefix(mount, old_path), new_name);
}

int vfs_delete(const char* path) {
    VFS_Mount* mount = vfs_get_mount(path);
    if (!mount || !mount->fs_driver->delete) return -1;
    return mount->fs_driver->delete(mount->root_node, vfs_strip_mount_prefix(mount, path));
}

void vfs_self_test(void) {
    if (vfs_get_mount("/") != NULL) {
        display_print("[SELF TEST] VFS: PASS (Root Mounted)\n");
    } else {
        display_print("[SELF TEST] VFS: FAILED (No Root Mount)\n");
    }
}
