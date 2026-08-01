#ifndef BSR_TYPES_H
#define BSR_TYPES_H

#include "kernel/botree/runtime/include/dre_types.h"
#include "kernel/desktop_runtime/include/bdr_types.h"

#define BSR_MAX_RUNTIMES      16
#define BSR_MAX_ASSOCIATIONS  32
#define BSR_MAX_RECENTS       50
#define BSR_MAX_FAVORITES     16

typedef enum {
    BSR_APP_EXPLORER = 0,
    BSR_APP_DESKTOP  = 1,
    BSR_APP_TERMINAL = 2,
    BSR_APP_DIALOG   = 3,
    BSR_APP_BROWSER  = 4
} BSR_AppType;

typedef struct {
    char ext[16];
    char app_name[64];
    char app_path[BDE_PATH_MAX];
} BSR_FileAssociation;

typedef struct {
    uint32_t shell_latency_us;
    uint32_t explorer_open_time_ms;
    uint32_t folder_switch_time_ms;
    uint32_t clipboard_ops_count;
    uint32_t drag_drop_count;
    uint32_t search_count;
    uint64_t memory_used_bytes;
} BSR_Diagnostics;

typedef struct BSR_Runtime {
    bool         active;
    uint32_t     runtime_id;
    uint32_t     owner_pid;
    uint32_t     active_window;
    BSR_AppType  app_type;

    BDeRuntime*  directory;    // DRE Runtime Engine Binding
    BDrSession*  desktop;      // BDR Desktop Session Binding

    char         current_path[BDE_PATH_MAX];
    uint32_t     focused_item;
    uint32_t     selected_count;

    bool         clipboard_active;
    bool         drag_active;
    bool         rename_active;
    bool         dialog_open;
    bool         search_active;
    bool         notification_active;
} BSR_Runtime;

#endif // BSR_TYPES_H
