#ifndef DRE_TYPES_H
#define DRE_TYPES_H

#include "kernel/botree/include/botree_types.h"

// Max Items per Runtime Instance
#define DRE_MAX_ITEMS          1024
#define DRE_MAX_BREADCRUMBS    16
#define DRE_MAX_SELECTIONS     256

// View Modes
typedef enum {
    DRE_VIEW_MODE_ICONS    = 0,
    DRE_VIEW_MODE_LIST     = 1,
    DRE_VIEW_MODE_DETAILS  = 2,
    DRE_VIEW_MODE_TILES    = 3
} BDeViewMode;

// Sort Fields
typedef enum {
    DRE_SORT_NAME      = 0,
    DRE_SORT_SIZE      = 1,
    DRE_SORT_TYPE      = 2,
    DRE_SORT_MODIFIED  = 3,
    DRE_SORT_EXTENSION = 4
} BDeSortField;

// Breadcrumb Segment Structure
typedef struct {
    char name[BDE_NAME_MAX];
    char target_path[BDE_PATH_MAX];
} BDeBreadcrumbSegment;

// Diagnostics Structure
typedef struct {
    uint32_t active_runtimes;
    uint32_t total_open_sessions;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint64_t avg_nav_time_us;
    uint64_t memory_used_bytes;
} BDeRuntimeDiagnostics;

// Core Runtime Instance Object
typedef struct BDeRuntime {
    bool         active;
    uint32_t     runtime_id;
    uint32_t     owner_pid;
    BDeNavHandle nav_session;
    BDeWatchHandle watch_handle;

    char current_dir[BDE_PATH_MAX];

    // Enumerated Directory Cache
    BDeDirEntry entries[DRE_MAX_ITEMS];
    uint32_t    item_count;

    // Selection State
    bool     selected_mask[DRE_MAX_ITEMS];
    int32_t  focused_index;
    int32_t  hovered_index;
    int32_t  active_index;

    // Sorting & Filtering Options
    BDeSortField sort_field;
    bool         sort_ascending;
    bool         show_hidden;
    char         extension_filter[64];

    // View Runtime Metrics
    BDeViewMode view_mode;
    int32_t     scroll_y;
    uint32_t    zoom_scale;

    // Breadcrumb Cache
    BDeBreadcrumbSegment breadcrumbs[DRE_MAX_BREADCRUMBS];
    uint32_t             breadcrumb_count;
} BDeRuntime;

#endif // DRE_TYPES_H
