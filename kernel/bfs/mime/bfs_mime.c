#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

const char* BFS_GetMime(const char* path) {
    if (!path) return "application/octet-stream";
    size_t len = strlen(path);
    if (len > 4 && strcmp(&path[len - 4], ".txt") == 0) return "text/plain";
    if (len > 3 && strcmp(&path[len - 3], ".md") == 0) return "text/markdown";
    if (len > 4 && strcmp(&path[len - 4], ".bmp") == 0) return "image/bmp";
    if (len > 4 && strcmp(&path[len - 4], ".png") == 0) return "image/png";
    if (len > 4 && strcmp(&path[len - 4], ".elf") == 0) return "application/x-elf";
    return "application/octet-stream";
}
