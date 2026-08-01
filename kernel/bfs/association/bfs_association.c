#include "../include/bfs_api.h"
#include "kernel/core/lib/include/string.h"

const char* BFS_GetAssociation(const char* extension) {
    if (!extension) return "notepad.elf";
    if (strcmp(extension, ".txt") == 0 || strcmp(extension, ".md") == 0) return "notepad.elf";
    if (strcmp(extension, ".bmp") == 0 || strcmp(extension, ".png") == 0) return "imgview.elf";
    if (strcmp(extension, ".elf") == 0) return "loader.elf";
    return "notepad.elf";
}
