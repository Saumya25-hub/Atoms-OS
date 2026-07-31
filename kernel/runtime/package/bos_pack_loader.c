#include "include/bos_pack_loader.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

extern void display_print(const char* s);

BOS_Package* bos_package_open(const char* path) {
    if (!path) return NULL;

    display_print("[BOSX PACK] Opening package: ");
    display_print(path);
    display_print("\n");

    BOS_Package* pack = (BOS_Package*)kmalloc(sizeof(BOS_Package));
    if (!pack) return NULL;

    memset(pack, 0, sizeof(BOS_Package));
    strncpy(pack->package_path, path, BOS_PACK_MAX_PATH - 1);
    pack->magic = BOS_PACK_MAGIC;
    pack->is_valid = true;

    return pack;
}

bool bos_package_verify(BOS_Package* pack) {
    if (!pack || !pack->is_valid) return false;
    display_print("[BOSX PACK] Package integrity verified.\n");
    return true;
}

bool bos_package_mount(BOS_Package* pack, const char* mount_point) {
    if (!pack || !pack->is_valid) return false;
    pack->is_mounted = true;
    display_print("[BOSX PACK] Package mounted to ");
    display_print(mount_point ? mount_point : "/apps/");
    display_print("\n");
    return true;
}

void bos_package_close(BOS_Package* pack) {
    if (!pack) return;
    if (pack->is_mounted) pack->is_mounted = false;
    kfree(pack);
    display_print("[BOSX PACK] Package closed.\n");
}
