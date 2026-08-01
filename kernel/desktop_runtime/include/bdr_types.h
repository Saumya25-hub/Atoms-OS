#ifndef BDR_TYPES_H
#define BDR_TYPES_H

#include "kernel/botree/runtime/include/dre_types.h"

#define BDR_MAX_ICONS          256
#define BDR_MAX_NOTIFICATIONS  16
#define BDR_GRID_CELL_WIDTH    80
#define BDR_GRID_CELL_HEIGHT   80

typedef enum {
    BDR_ICON_TYPE_FILE       = 0,
    BDR_ICON_TYPE_FOLDER     = 1,
    BDR_ICON_TYPE_SHORTCUT   = 2,
    BDR_ICON_TYPE_THIS_PC    = 3,
    BDR_ICON_TYPE_RECYCLE    = 4,
    BDR_ICON_TYPE_DRIVE      = 5
} BDrIconType;

typedef enum {
    BDR_WALLPAPER_SOLID    = 0,
    BDR_WALLPAPER_STRETCH  = 1,
    BDR_WALLPAPER_FIT      = 2,
    BDR_WALLPAPER_CENTER   = 3,
    BDR_WALLPAPER_TILE     = 4
} BDrWallpaperMode;

typedef enum {
    BDR_NOTIF_INFO    = 0,
    BDR_NOTIF_SUCCESS = 1,
    BDR_NOTIF_WARNING = 2,
    BDR_NOTIF_ERROR   = 3
} BDrNotificationType;

typedef struct {
    uint32_t    icon_id;
    char        name[BDE_NAME_MAX];
    char        target_path[BDE_PATH_MAX];
    BDrIconType type;
    int32_t     grid_x;
    int32_t     grid_y;
    int32_t     pixel_x;
    int32_t     pixel_y;
    bool        selected;
    bool        focused;
} BDrIconNode;

typedef struct {
    char                title[64];
    char                message[128];
    BDrNotificationType type;
    uint64_t            timestamp;
    bool                active;
} BDrNotification;

typedef struct {
    uint32_t            active_icons;
    uint32_t            selected_icons;
    uint32_t            grid_utilization_pct;
    uint32_t            active_notifications;
    uint64_t            memory_bytes;
} BDrDiagnostics;

typedef struct BDrSession {
    bool             active;
    uint32_t         session_id;
    uint32_t         user_id;

    BDeRuntime*      dre_runtime;

    BDrIconNode      icons[BDR_MAX_ICONS];
    uint32_t         icon_count;

    // Workspace & Marquee Selection
    bool             is_selecting_box;
    int32_t          box_x1;
    int32_t          box_y1;
    int32_t          box_x2;
    int32_t          box_y2;

    // Wallpaper
    char             wallpaper_path[BDE_PATH_MAX];
    uint32_t         solid_color;
    BDrWallpaperMode wallpaper_mode;

    // Notifications Queue
    BDrNotification  notifications[BDR_MAX_NOTIFICATIONS];
    uint32_t         notification_count;
} BDrSession;

#endif // BDR_TYPES_H
