#ifndef STUDIO_TOOLBOX_H
#define STUDIO_TOOLBOX_H

#include "platform/include/bos_types.h"

typedef struct {
    char category[32];
    char control_type[32];
    char icon_res_id[32];
} BOS_ToolboxItem;

void            BOS_Studio_ToolboxInit(void);
uint32_t        BOS_Studio_ToolboxGetCount(void);
BOS_ToolboxItem BOS_Studio_ToolboxGetItem(uint32_t index);

#endif /* STUDIO_TOOLBOX_H */
