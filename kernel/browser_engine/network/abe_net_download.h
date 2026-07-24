#ifndef ABE_NET_DOWNLOAD_H
#define ABE_NET_DOWNLOAD_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_DOWNLOAD_SLOTS 16
#define ABE_DOWNLOAD_BUFFER_SIZE (64 * 1024)

typedef enum {
    DOWNLOAD_STATE_IDLE = 0,
    DOWNLOAD_STATE_CONNECTING = 1,
    DOWNLOAD_STATE_DOWNLOADING = 2,
    DOWNLOAD_STATE_PAUSED = 3,
    DOWNLOAD_STATE_COMPLETED = 4,
    DOWNLOAD_STATE_CANCELLED = 5,
    DOWNLOAD_STATE_FAILED = 6
} ABE_DownloadStateEnum;

typedef struct {
    ABE_DownloadHandle handle;
    char target_url[ABE_MAX_URL_LEN];
    char destination_filename[256];
    size_t bytes_downloaded;
    size_t total_bytes;
    uint32_t throughput_bps;
    ABE_DownloadStateEnum state;
    ABE_DownloadProgressCallback progress_cb;
    void* user_data;
    uint8_t ring_buffer[ABE_DOWNLOAD_BUFFER_SIZE];
    size_t ring_buffer_bytes;
    bool is_resumable;
} ABE_DownloadStream;

typedef struct {
    ABE_DownloadStream streams[ABE_MAX_DOWNLOAD_SLOTS];
    uint32_t active_downloads_count;
} ABE_DownloadManager;

ABE_Error ABE_NetDownload_Init(void);
ABE_Error ABE_NetDownload_Shutdown(void);

ABE_Error ABE_NetDownload_Start(const char* url, ABE_DownloadProgressCallback cb, void* user_data, ABE_DownloadHandle* out_handle);
ABE_Error ABE_NetDownload_Cancel(ABE_DownloadHandle handle);
ABE_Error ABE_NetDownload_Pause(ABE_DownloadHandle handle);
ABE_Error ABE_NetDownload_Resume(ABE_DownloadHandle handle);

ABE_DownloadStream* ABE_NetDownload_Get(ABE_DownloadHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_DOWNLOAD_H
