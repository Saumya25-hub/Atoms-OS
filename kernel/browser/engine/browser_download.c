#include "browser_download.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ATRIX_DownloadEngine_Init(void) {
    bwe_log("INFO", "ATRIX Real NTFS Download Engine Subsystem Initialized");
}

bool ATRIX_DownloadEngine_SaveToNTFS(const char* filename, const void* data, uint32_t len) {
    if (!filename || !data || len == 0) return false;

    int fd = vfs_open(filename);
    if (fd >= 0) {
        vfs_write(fd, (void*)data, len);
        vfs_close(fd);
        bwe_log("INFO", "ATRIX Download Saved to File System Successfully");
        return true;
    }
    return false;
}
