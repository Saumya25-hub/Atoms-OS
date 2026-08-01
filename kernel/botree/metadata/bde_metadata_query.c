#include "../include/botree.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"

int32_t BDe_GetMetadata(const char* path, BDeDirEntry* out_stat) {
    if (!path || !out_stat) return -1;

    memset(out_stat, 0, sizeof(BDeDirEntry));
    BDe_PathGetBasename(path, out_stat->name, BDE_NAME_MAX);
    strcpy(out_stat->full_path, path);

    char dir_path[BDE_PATH_MAX];
    char base_name[BDE_NAME_MAX];
    if (BDe_PathGetDirname(path, dir_path, BDE_PATH_MAX) != 0) strcpy(dir_path, "/");
    if (BDe_PathGetBasename(path, base_name, BDE_NAME_MAX) != 0) strcpy(base_name, "");

    vfs_dirent_t dirent;
    int idx = 0;
    while (vfs_readdir(dir_path, idx++, &dirent) == 0) {
        if (strcmp(dirent.name, base_name) == 0) {
            out_stat->size_bytes = dirent.size;
            out_stat->is_directory = dirent.is_directory;
            out_stat->attributes = dirent.is_directory ? BDE_ATTR_DIRECTORY : 0;
            return 0;
        }
    }

    // Default metadata if root or not found
    if (strcmp(path, "/") == 0) {
        out_stat->is_directory = true;
        out_stat->attributes = BDE_ATTR_DIRECTORY;
        return 0;
    }
    return -1;
}
