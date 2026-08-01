#ifndef BOS_EXPLORER_TYPES_H
#define BOS_EXPLORER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"
#include "userspace/libs/user32/include/user32_types.h"

typedef enum {
    EXPLORER_SESSION_STOPPED = 0,
    EXPLORER_SESSION_STARTING,
    EXPLORER_SESSION_RUNNING,
    EXPLORER_SESSION_LOCKED,
    EXPLORER_SESSION_LOGGING_OUT
} EXPLORER_SESSION_STATE;

typedef struct _EXPLORER_DESKTOP_ICON {
    char name[64];
    char target_path[256];
    uint32_t icon_id;
    int32_t x;
    int32_t y;
    bool is_selected;
} EXPLORER_DESKTOP_ICON;

typedef struct _EXPLORER_TASKBAR_ITEM {
    HANDLE hwnd;
    char title[64];
    uint32_t icon_id;
    bool is_active;
    bool is_pinned;
} EXPLORER_TASKBAR_ITEM;

#endif // BOS_EXPLORER_TYPES_H
