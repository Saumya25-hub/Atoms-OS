#include "kernel/vfs/include/dummy_fs.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/display/display.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/string.h"

static VFS_Node* dummy_mount(BlockDevice* device) {
    (void)device;
    // Create a dummy root node
    VFS_Node* root = (VFS_Node*)kmalloc(sizeof(VFS_Node));
    if (!root) return NULL;
    
    strcpy(root->name, "/");
    root->type = VFS_MOUNTPOINT;
    root->size = 0;
    root->parent = NULL;
    // Private data could point to FAT32 internal state in the real driver.
    root->private_data = NULL;
    
    // Set the driver so vfs_read/vfs_close can find it later
    extern FilesystemDriver dummy_fs_driver; // forward declare
    root->fs_driver = &dummy_fs_driver;

    return root;
}

static int dummy_open(VFS_Node* node, const char* path) {
    (void)node;
    display_print("[VFS] Open(\"");
    display_print(path);
    display_print("\")\n");
    return 0; // Success
}

static int dummy_read(VFS_Node* node, uint64_t offset, uint32_t size, void* buffer) {
    (void)node;
    (void)offset;
    (void)size;
    (void)buffer;
    display_print("[VFS] Read()\n");
    return 0; // Success
}

static int dummy_close(VFS_Node* node) {
    (void)node;
    display_print("[VFS] Close()\n");
    return 0; // Success
}

FilesystemDriver dummy_fs_driver = {
    .name = "dummyfs",
    .mount = dummy_mount,
    .open = dummy_open,
    .read = dummy_read,
    .close = dummy_close
};

void dummyfs_init(void) {
    // 1. Register the dummy filesystem driver with VFS
    vfs_register_fs(&dummy_fs_driver);
}
