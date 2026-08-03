#ifndef KERNEL_BOASSET_ASSET_TYPES_H
#define KERNEL_BOASSET_ASSET_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/ui/boimage/boimage.h"

// Error codes
#define BOASSET_OK                    0
#define BOASSET_ERROR_NOT_FOUND      -1
#define BOASSET_ERROR_INVALID_IMAGE  -2
#define BOASSET_ERROR_OUT_OF_MEMORY  -3

// Asset Types
typedef enum {
    ASSET_TYPE_IMAGE = 0,
    ASSET_TYPE_ICON = 1,
    ASSET_TYPE_CURSOR = 2,
    ASSET_TYPE_WALLPAPER = 3
} BOAssetType;

// Predefined System Asset IDs
#define BOASSET_ID_NONE          0
#define ICON_FOLDER              101
#define ICON_FILE                102
#define ICON_TERMINAL            103
#define ICON_EXPLORER            104
#define ICON_SETTINGS            105
#define ICON_CLOSE               106
#define ICON_MINIMIZE            107
#define ICON_MAXIMIZE            108
#define ICON_CALCULATOR          109
#define ICON_STRESS_TEST         110
#define ICON_MUSIC               111
#define ICON_DOOM                112
#define ICON_INPUT_LAB           113
#define ICON_ATRIX               114
#define ICON_GRAPH_3D            115
#define ICON_TMH                 116
#define ICON_RECYCLE_BIN         117
#define ICON_USB_DISK            118
#define CURSOR_ARROW             201
#define CURSOR_HAND              202
#define CURSOR_TEXT              203
#define ASSET_LOGO               301
#define ASSET_WALLPAPER          302

// System Status Bar Icon Asset IDs V1.1 (Isolated Namespace)
#define ICON_SYS_WIFI_CONN       401
#define ICON_SYS_WIFI_WEAK       402
#define ICON_SYS_WIFI_DISC       403
#define ICON_SYS_VOL_NORM        404
#define ICON_SYS_VOL_LOW         405
#define ICON_SYS_VOL_MUTE        406
#define ICON_SYS_BAT_NORM        407
#define ICON_SYS_BAT_CHG         408
#define ICON_SYS_BAT_LOW         409
#define ICON_SYS_BELL_NORM       410
#define ICON_SYS_BELL_UNREAD     411

// Asset Handle Object Model
typedef struct BOAssetHandle {
    uint32_t id;
    BOAssetType type;
    char filepath[128];
    uint32_t width;
    uint32_t height;
    bool loaded;
    uint32_t ref_count;
    BOImage* image_data;
    // Normalized texture atlas coordinates for BOIMAGE v2 batch renderer
    float u1;
    float v1;
    float u2;
    float v2;
} BOAssetHandle;

#endif // KERNEL_BOASSET_ASSET_TYPES_H
