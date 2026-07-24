#include "app_vfs_api.h"
#include "kernel/application/permissions/app_permissions.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"

int32_t ATOMS_VFS_OpenFile(uint32_t app_id, const char* path, const char* mode) {
    (void)mode;
    if (!ATOMS_CheckCapability(app_id, ATOMS_CAPABILITY_STORAGE_READ)) {
        return -1; // Permission Denied
    }
    return vfs_open(path);
}

int32_t ATOMS_VFS_ReadFile(int32_t handle, void* buffer, uint32_t size) {
    if (handle < 0 || !buffer) return -1;
    return vfs_read(handle, buffer, size);
}

int32_t ATOMS_VFS_WriteFile(int32_t handle, const void* buffer, uint32_t size) {
    if (handle < 0 || !buffer) return -1;
    return vfs_write(handle, (void*)buffer, size);
}

void ATOMS_VFS_CloseFile(int32_t handle) {
    if (handle >= 0) {
        vfs_close(handle);
    }
}

bool ATOMS_VFS_ListDirectory(uint32_t app_id, const char* path, ATOMS_VFS_FileInfo* out_entries, uint32_t max_entries, uint32_t* out_count) {
    if (!ATOMS_CheckCapability(app_id, ATOMS_CAPABILITY_STORAGE_READ)) {
        return false;
    }
    (void)path;
    (void)out_entries;
    (void)max_entries;
    if (out_count) *out_count = 0;
    return true;
}
