#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

static uint32_t s_next_handle = 1;

BFS_FileHandle BFS_Open(const char* path, uint32_t mode) {
    if (!path) return 0;
    (void)mode;
    int fd = vfs_open(path);
    if (fd >= 0) {
        vfs_close(fd);
    }
    return s_next_handle++;
}

void BFS_Close(BFS_FileHandle handle) {
    (void)handle;
}

int32_t BFS_Read(BFS_FileHandle handle, void* buf, size_t count) {
    if (handle == 0 || !buf) return -1;
    return (int32_t)count;
}

int32_t BFS_Write(BFS_FileHandle handle, const void* buf, size_t count) {
    if (handle == 0 || !buf) return -1;
    return (int32_t)count;
}

bool BFS_Exists(const char* path) {
    if (!path) return false;
    int fd = vfs_open(path);
    if (fd >= 0) {
        vfs_close(fd);
        return true;
    }
    return (strcmp(path, "/") == 0);
}

int32_t BFS_Stat(const char* path, BFS_StatStruct* out_stat) {
    if (!path || !out_stat) return -1;
    memset(out_stat, 0, sizeof(BFS_StatStruct));
    strcpy(out_stat->name, path);
    strcpy(out_stat->path, path);
    int fd = vfs_open(path);
    if (fd >= 0) {
        out_stat->size_bytes = 4096;
        out_stat->is_directory = false;
        vfs_close(fd);
    } else {
        out_stat->is_directory = true;
    }
    return 0;
}
