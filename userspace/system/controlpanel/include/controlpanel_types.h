#ifndef BOS_CONTROLPANEL_TYPES_H
#define BOS_CONTROLPANEL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "userspace/libs/kernel32/include/kernel32_types.h"

typedef struct _BOSC_MODULE_INFO {
    char module_name[64];
    char display_name[64];
    char category[32];
    uint32_t icon_id;
    uint32_t version;
    bool is_pinned;
    bool is_installed;
} BOSC_MODULE_INFO;

#endif // BOS_CONTROLPANEL_TYPES_H
