#ifndef BRT_TYPES_H
#define BRT_TYPES_H

#include "kernel/shell_runtime/include/bsr_types.h"

#define BRT_MAX_RUNTIMES    32
#define BRT_MAX_OBJECTS     512
#define BRT_MAX_SUBSCRIBERS 32
#define BRT_MAX_EVENTS      16

typedef enum {
    BRT_APP_EXPLORER   = 0,
    BRT_APP_DESKTOP    = 1,
    BRT_APP_TERMINAL   = 2,
    BRT_APP_BROWSER    = 3,
    BRT_APP_AI         = 4,
    BRT_APP_DIALOG     = 5,
    BRT_APP_SETTINGS   = 6,
    BRT_APP_MEDIA      = 7,
    BRT_APP_DEVTOOLS   = 8
} BRT_AppType;

typedef enum {
    BRT_OBJ_FILE       = 0,
    BRT_OBJ_DIRECTORY  = 1,
    BRT_OBJ_VIRTUAL    = 2,
    BRT_OBJ_DEVICE     = 3
} BRTObjectType;

typedef enum {
    BRT_TX_COPY   = 0,
    BRT_TX_MOVE   = 1,
    BRT_TX_DELETE = 2,
    BRT_TX_RENAME = 3
} BRTTxType;

typedef uint32_t BRTTxHandle;

typedef void (*BRTEventCallback)(uint32_t event_id, void* payload);

typedef struct {
    uint32_t      object_id;
    char          name[BDE_NAME_MAX];
    char          path[BDE_PATH_MAX];
    BRTObjectType type;
    uint32_t      ref_count;
    uint32_t      owner_pid;
    uint32_t      security_flags;
} BRTObject;

typedef struct {
    uint32_t api_latency_us;
    uint32_t active_runtimes;
    uint32_t active_objects;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t lock_contention_count;
    uint64_t memory_used_bytes;
} BRT_Diagnostics;

typedef struct BRTRuntime {
    bool         active;
    uint32_t     runtime_id;
    uint32_t     owner_pid;
    uint32_t     window_id;
    BRT_AppType  app_type;

    BSR_Runtime* shell_rt;  // BSR Master Authority Binding

    char         current_path[BDE_PATH_MAX];
    uint32_t     ref_count;

    // Drag Session State
    bool         is_dragging;
    int32_t      drag_start_x;
    int32_t      drag_start_y;
    int32_t      drag_cur_x;
    int32_t      drag_cur_y;

    // Transaction State
    BRTTxHandle  active_tx;
} BRTRuntime;

#endif // BRT_TYPES_H
