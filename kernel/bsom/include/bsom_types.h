#ifndef BSOM_TYPES_H
#define BSOM_TYPES_H

#include "kernel/bfs/include/bfs_types.h"

#define BSOM_MAX_OBJECTS     1024
#define BSOM_MAX_HANDLES     512
#define BSOM_MAX_PROPERTIES  32
#define BSOM_MAX_MENU_ITEMS  16

typedef uint32_t BSOMHandle;

typedef enum {
    BSOM_CLASS_FILE          = 1,
    BSOM_CLASS_FOLDER        = 2,
    BSOM_CLASS_DRIVE         = 3,
    BSOM_CLASS_USB           = 4,
    BSOM_CLASS_SHORTCUT      = 5,
    BSOM_CLASS_APP           = 6,
    BSOM_CLASS_IMAGE         = 7,
    BSOM_CLASS_DOCUMENT      = 8,
    BSOM_CLASS_AUDIO         = 9,
    BSOM_CLASS_VIDEO         = 10,
    BSOM_CLASS_VIRTUAL       = 11,
    BSOM_CLASS_NETWORK       = 12,
    BSOM_CLASS_SEARCH        = 13,
    BSOM_CLASS_RECENT         = 14,
    BSOM_CLASS_FAVORITE      = 15,
    BSOM_CLASS_RECYCLE       = 16,
    BSOM_CLASS_CLIPBOARD     = 17,
    BSOM_CLASS_DRAG          = 18,
    BSOM_CLASS_THUMBNAIL     = 19,
    BSOM_CLASS_ICON          = 20,
    BSOM_CLASS_PROPERTY      = 21,
    BSOM_CLASS_PERMISSION    = 22,
    BSOM_CLASS_TRANSACTION   = 23,
    BSOM_CLASS_CONTEXTMENU   = 24,
    BSOM_CLASS_AI_WORKSPACE  = 25,
    BSOM_CLASS_QUICKACCESS   = 26
} BSOMClassType;

typedef struct {
    char key[64];
    char value[128];
} BSOMProperty;

typedef struct {
    uint32_t command_id;
    char     label[64];
    bool     enabled;
} BSOMContextMenuItem;

typedef struct {
    uint32_t            item_count;
    BSOMContextMenuItem items[BSOM_MAX_MENU_ITEMS];
} BSOMContextMenu;

typedef struct BSOMObject {
    uint32_t      object_id;
    char          name[BDE_NAME_MAX];
    char          path[BDE_PATH_MAX];
    BSOMClassType class_type;
    uint32_t      ref_count;
    uint32_t      owner_pid;
    uint32_t      icon_id;
    bool          is_virtual;

    // Property Table
    uint32_t      prop_count;
    BSOMProperty  properties[BSOM_MAX_PROPERTIES];
} BSOMObject;

typedef void (*BSOMEnumCallback)(BSOMObject* obj, void* user_data);

typedef struct {
    uint32_t active_objects;
    uint32_t active_handles;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t api_latency_us;
    uint64_t memory_used_bytes;
} BSOM_Diagnostics;

#endif // BSOM_TYPES_H
