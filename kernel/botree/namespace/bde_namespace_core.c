#include "../include/botree_namespace.h"
#include "../include/botree_path.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

bool BDe_NamespaceIsVirtual(const char* path) {
    if (!path) return false;
    return (strncmp(path, "virtual://", 10) == 0);
}

int32_t BDe_NamespaceResolve(const char* virtual_uri, char* out_physical_path, size_t max_len) {
    if (!virtual_uri || !out_physical_path || max_len == 0) return -1;

    if (!BDe_NamespaceIsVirtual(virtual_uri)) {
        return BDe_PathNormalize(virtual_uri, out_physical_path, max_len);
    }

    if (strcmp(virtual_uri, BDE_URI_THIS_PC) == 0) {
        strcpy(out_physical_path, "/");
    } else if (strcmp(virtual_uri, BDE_URI_DESKTOP) == 0) {
        strcpy(out_physical_path, "/DESKTOP");
    } else if (strcmp(virtual_uri, BDE_URI_DOCUMENTS) == 0) {
        strcpy(out_physical_path, "/DOCS");
    } else if (strcmp(virtual_uri, BDE_URI_DOWNLOADS) == 0) {
        strcpy(out_physical_path, "/DOWNLOAD");
    } else if (strcmp(virtual_uri, BDE_URI_PICTURES) == 0) {
        strcpy(out_physical_path, "/PHOTO");
    } else if (strcmp(virtual_uri, BDE_URI_MUSIC) == 0) {
        strcpy(out_physical_path, "/MUSIC");
    } else if (strcmp(virtual_uri, BDE_URI_VIDEOS) == 0) {
        strcpy(out_physical_path, "/VIDEO");
    } else if (strcmp(virtual_uri, BDE_URI_RECYCLE_BIN) == 0) {
        strcpy(out_physical_path, "/RECYCLE");
    } else if (strcmp(virtual_uri, BDE_URI_USB) == 0) {
        strcpy(out_physical_path, "/U");
    } else {
        strcpy(out_physical_path, "/");
    }
    return 0;
}

int32_t BDe_NamespaceGetObjects(const char* namespace_uri, BDeDirEntry** out_entries, uint32_t* out_count) {
    if (!out_entries || !out_count) return -1;

    // Handle "This PC" Virtual Merged Namespace Object List
    if (!namespace_uri || strcmp(namespace_uri, BDE_URI_THIS_PC) == 0 || strcmp(namespace_uri, "/") == 0) {
        static const struct {
            const char* name;
            const char* uri;
            uint32_t    icon_role;
        } c_this_pc_objects[] = {
            {"Desktop", BDE_URI_DESKTOP, 1},
            {"Documents", BDE_URI_DOCUMENTS, 2},
            {"Downloads", BDE_URI_DOWNLOADS, 3},
            {"Pictures", BDE_URI_PICTURES, 4},
            {"Music", BDE_URI_MUSIC, 5},
            {"Videos", BDE_URI_VIDEOS, 6},
            {"Local Disk (C:)", "/C", 7},
            {"USB Drive (U:)", BDE_URI_USB, 8},
            {"Recycle Bin", BDE_URI_RECYCLE_BIN, 9},
            {"Control Panel", BDE_URI_CONTROL_PANEL, 10},
            {"Settings", BDE_URI_SETTINGS, 11}
        };

        uint32_t count = 11;
        BDeDirEntry* entries = (BDeDirEntry*)kcalloc(count, sizeof(BDeDirEntry));
        if (!entries) return -1;

        for (uint32_t i = 0; i < count; i++) {
            strcpy(entries[i].name, c_this_pc_objects[i].name);
            strcpy(entries[i].full_path, c_this_pc_objects[i].uri);
            entries[i].is_directory = true;
            entries[i].is_virtual = true;
            entries[i].attributes = BDE_ATTR_DIRECTORY | BDE_ATTR_VIRTUAL;
            entries[i].icon_role = c_this_pc_objects[i].icon_role;
        }

        *out_entries = entries;
        *out_count = count;
        return 0;
    }

    // Default physical fallback resolution
    extern int32_t BDe_ReadDirectory(const char* path, BDeDirEntry** out_entries, uint32_t* out_count);
    char physical[BDE_PATH_MAX];
    BDe_NamespaceResolve(namespace_uri, physical, BDE_PATH_MAX);
    return BDe_ReadDirectory(physical, out_entries, out_count);
}
