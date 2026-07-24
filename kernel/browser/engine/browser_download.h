#ifndef ATRIX_BROWSER_DOWNLOAD_H
#define ATRIX_BROWSER_DOWNLOAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Real NTFS Download Engine Subsystem
// ============================================================

typedef struct {
    char     filename[64];
    uint32_t total_bytes;
    uint32_t received_bytes;
    bool     is_completed;
} ATRIX_DownloadTask;

void ATRIX_DownloadEngine_Init(void);
bool ATRIX_DownloadEngine_SaveToNTFS(const char* filename, const void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_DOWNLOAD_H
