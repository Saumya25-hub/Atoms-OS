#include "../include/botree.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

int32_t BDe_ReadDirectory(const char* path, BDeDirEntry** out_entries, uint32_t* out_count) {
    if (!out_entries || !out_count) return -1;

    char norm[BDE_PATH_MAX];
    if (BDe_PathNormalize(path ? path : "/", norm, BDE_PATH_MAX) != 0) return -1;

    // Check if namespace is virtual first
    if (BDe_NamespaceIsVirtual(norm)) {
        return BDe_NamespaceGetObjects(norm, out_entries, out_count);
    }

    // Direct read from VFS into unified BDeDirEntry structures
    vfs_dirent_t entry;
    int index = 0;
    uint32_t capacity = 16;
    uint32_t count = 0;

    BDeDirEntry* entries = (BDeDirEntry*)kcalloc(capacity, sizeof(BDeDirEntry));
    if (!entries) return -1;

    while (vfs_readdir(norm, index, &entry) == 0 && count < BDE_MAX_ENTRIES) {
        if (strlen(entry.name) > 0) {
            if (count >= capacity) {
                uint32_t new_cap = capacity * 2;
                BDeDirEntry* new_entries = (BDeDirEntry*)kcalloc(new_cap, sizeof(BDeDirEntry));
                if (!new_entries) {
                    kfree(entries);
                    return -1;
                }
                memcpy(new_entries, entries, count * sizeof(BDeDirEntry));
                kfree(entries);
                entries = new_entries;
                capacity = new_cap;
            }

            BDeDirEntry* item = &entries[count++];
            strcpy(item->name, entry.name);
            BDe_PathJoin(norm, entry.name, item->full_path, BDE_PATH_MAX);
            item->is_directory = entry.is_directory;
            item->is_virtual = false;
            item->size_bytes = entry.size;
            item->attributes = entry.is_directory ? BDE_ATTR_DIRECTORY : 0;
        }
        index++;
    }

    // Fallback if VFS returned 0 items for root "/" -> Use Virtual Namespace objects
    if (count == 0 && strcmp(norm, "/") == 0) {
        kfree(entries);
        return BDe_NamespaceGetObjects(BDE_URI_THIS_PC, out_entries, out_count);
    }

    *out_entries = entries;
    *out_count = count;
    return 0;
}

void BDe_FreeDirectoryListing(BDeDirEntry* entries) {
    if (entries) {
        kfree(entries);
    }
}

void BDe_InvalidateCache(const char* path) {
    (void)path;
    // Invalidate Dentry and Directory Caches
}
