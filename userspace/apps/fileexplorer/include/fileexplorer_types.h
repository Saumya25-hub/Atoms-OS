#ifndef BOS_FILEEXPLORER_TYPES_H
#define BOS_FILEEXPLORER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define FE_MAX_PATH          512
#define FE_MAX_FILENAME      256
#define FE_MAX_ENTRIES       1024
#define FE_MAX_HISTORY       256
#define FE_MAX_FAVORITES     128
#define FE_MAX_RECENT        64
#define FE_MAX_OPS           256
#define FE_THUMB_CACHE_MAX   4096
#define FE_SEARCH_MAX_RESULTS 8192
#define FE_MAX_WATCHERS      64

// ---- Filesystem Types ----
typedef enum {
    FS_UNKNOWN = 0,
    FS_BFS,
    FS_NTFS,
    FS_FAT32,
    FS_USB,
    FS_CD_DVD,
    FS_RAM_DISK,
    FS_NETWORK,
    FS_VIRTUAL,
} FE_FS_TYPE;

// ---- View Modes ----
typedef enum {
    VIEW_EXTRA_LARGE_ICONS = 0,
    VIEW_LARGE_ICONS,
    VIEW_MEDIUM_ICONS,
    VIEW_SMALL_ICONS,
    VIEW_LIST,
    VIEW_DETAILS,
    VIEW_TILES,
    VIEW_CONTENT,
} FE_VIEW_MODE;

// ---- File Entry ----
typedef struct _FE_ENTRY {
    char         name[FE_MAX_FILENAME];
    char         path[FE_MAX_PATH];
    bool         is_dir;
    uint64_t     size_bytes;
    uint32_t     attributes;
    uint64_t     created_ts;
    uint64_t     modified_ts;
    uint64_t     accessed_ts;
    char         extension[16];
    char         fs_type[8];
    uint32_t     open_handle_count;
    char         owner[64];
    char         sha256[65];      // Forensic mode: SHA-256 hex
    bool         is_signed;       // Forensic: digital signature
    bool         is_encrypted;
    bool         is_compressed;
    uint64_t     file_id;         // Forensic: filesystem file ID
    uint64_t     volume_id;       // Forensic: volume ID
    uint32_t     cluster_map;     // Forensic: logical cluster
    uint32_t     read_latency_us; // Forensic: read latency
    uint32_t     write_latency_us;
    char         locked_by[64];   // Forensic: process holding lock
} FE_ENTRY;

// ---- Drive Entry ----
typedef struct _FE_DRIVE {
    char       label[32];
    char       path[8];
    FE_FS_TYPE fs_type;
    uint64_t   total_bytes;
    uint64_t   free_bytes;
    bool       is_removable;
    bool       is_network;
    bool       is_ready;
} FE_DRIVE;

// ---- File Operation ----
typedef enum {
    FE_OP_COPY = 0,
    FE_OP_MOVE,
    FE_OP_DELETE,
    FE_OP_RENAME,
    FE_OP_CREATE_DIR,
    FE_OP_CREATE_FILE,
    FE_OP_COMPRESS,
    FE_OP_EXTRACT,
    FE_OP_RESTORE,
} FE_OP_TYPE;

typedef enum {
    FE_OP_PENDING = 0,
    FE_OP_RUNNING,
    FE_OP_PAUSED,
    FE_OP_DONE,
    FE_OP_FAILED,
    FE_OP_CANCELLED,
} FE_OP_STATUS;

typedef struct _FE_OPERATION {
    uint32_t    id;
    FE_OP_TYPE  type;
    FE_OP_STATUS status;
    char        src[FE_MAX_PATH];
    char        dst[FE_MAX_PATH];
    uint64_t    bytes_total;
    uint64_t    bytes_done;
    uint32_t    progress_pct;
    bool        verified;
    char        checksum_sha256[65];
    bool        conflict;
    char        conflict_resolution[32];
} FE_OPERATION;

// ---- Search Query ----
typedef struct _FE_SEARCH_QUERY {
    char    pattern[FE_MAX_FILENAME];
    char    extension[16];
    uint64_t min_size;
    uint64_t max_size;
    uint64_t modified_after;
    uint64_t modified_before;
    char    owner[64];
    char    content[256];
    bool    recursive;
} FE_SEARCH_QUERY;

// ---- Navigation History Entry ----
typedef struct _FE_HISTORY_ENTRY {
    char path[FE_MAX_PATH];
    uint64_t timestamp;
} FE_HISTORY_ENTRY;

// ---- Diagnostics ----
typedef struct _FE_DIAGNOSTICS {
    uint32_t folder_load_time_ms;
    uint32_t thumb_cache_hits;
    uint32_t thumb_cache_misses;
    uint32_t bfs_read_latency_us;
    uint32_t ntfs_read_latency_us;
    uint32_t fat32_read_latency_us;
    uint32_t active_ops;
    uint32_t search_index_time_ms;
    uint32_t preview_render_time_ms;
    uint32_t ole_ext_time_ms;
    uint32_t context_menu_build_time_ms;
    uint32_t clipboard_queue_depth;
    uint32_t watcher_events_per_sec;
    uint64_t memory_used_bytes;
    uint32_t handle_count;
} FE_DIAGNOSTICS;

#endif // BOS_FILEEXPLORER_TYPES_H
