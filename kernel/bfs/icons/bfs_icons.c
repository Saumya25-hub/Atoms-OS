#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

uint32_t BFS_GetIcon(const char* path) {
    if (!path) return 1;
    if (strcmp(path, "/") == 0 || strcmp(path, "virtual://ThisPC") == 0) return 100;
    size_t len = strlen(path);
    if (len > 4 && strcmp(&path[len - 4], ".bmp") == 0) return 200;
    if (len > 4 && strcmp(&path[len - 4], ".elf") == 0) return 300;
    return 1; // Generic document icon
}
