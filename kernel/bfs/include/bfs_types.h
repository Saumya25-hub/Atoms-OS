#ifndef BFS_TYPES_H
#define BFS_TYPES_H

#include "kernel/botree/include/botree_types.h"

#define BFS_MAX_HANDLES      64
#define BFS_MAX_ITEMS        256
#define BFS_MAX_FAVORITES    32
#define BFS_MAX_RECENT       64

typedef uint32_t BFS_FileHandle;
typedef uint32_t BFSTxHandle;

typedef enum {
    BFS_OPEN_READ   = 0x01,
    BFS_OPEN_WRITE  = 0x02,
    BFS_OPEN_CREATE = 0x04,
    BFS_OPEN_APPEND = 0x08
} BFS_OpenMode;

typedef enum {
    BFS_LOCK_SHARED    = 1,
    BFS_LOCK_EXCLUSIVE = 2
} BFS_LockType;

typedef enum {
    BFS_TX_COPY   = 1,
    BFS_TX_MOVE   = 2,
    BFS_TX_DELETE = 3,
    BFS_TX_RENAME = 4
} BFSTxType;

typedef struct {
    char     name[BDE_NAME_MAX];
    char     path[BDE_PATH_MAX];
    uint64_t size_bytes;
    bool     is_directory;
    uint32_t attributes;
    uint64_t created_time;
    uint64_t modified_time;
    uint64_t accessed_time;
} BFS_StatStruct;

typedef struct {
    char     path[BDE_PATH_MAX];
    char     mime_type[64];
    uint32_t icon_id;
    uint64_t size_bytes;
    bool     is_directory;
    uint64_t created_time;
    uint64_t modified_time;
} BFS_Properties;

typedef struct {
    char     name[BDE_NAME_MAX];
    char     path[BDE_PATH_MAX];
    bool     is_directory;
    uint32_t icon_id;
} BFS_ItemEntry;

typedef struct {
    uint32_t open_handles;
    uint32_t active_transactions;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t transfer_rate_kbps;
    uint32_t lock_contention_count;
    uint64_t memory_used_bytes;
} BFS_Diagnostics;

#endif // BFS_TYPES_H
