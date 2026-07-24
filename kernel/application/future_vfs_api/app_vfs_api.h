#ifndef ATOMS_APP_VFS_API_H
#define ATOMS_APP_VFS_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Application VFS API Interface
// ============================================================

typedef struct {
    char     name[256];
    uint32_t size;
    bool     is_directory;
} ATOMS_VFS_FileInfo;

int32_t ATOMS_VFS_OpenFile(uint32_t app_id, const char* path, const char* mode);
int32_t ATOMS_VFS_ReadFile(int32_t handle, void* buffer, uint32_t size);
int32_t ATOMS_VFS_WriteFile(int32_t handle, const void* buffer, uint32_t size);
void    ATOMS_VFS_CloseFile(int32_t handle);
bool    ATOMS_VFS_ListDirectory(uint32_t app_id, const char* path, ATOMS_VFS_FileInfo* out_entries, uint32_t max_entries, uint32_t* out_count);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_VFS_API_H
