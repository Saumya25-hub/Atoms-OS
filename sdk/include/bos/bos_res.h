#ifndef BOS_RES_H
#define BOS_RES_H

#include "platform/include/bos_types.h"

typedef enum {
    BOS_RES_TYPE_IMAGE = 0,
    BOS_RES_TYPE_ICON,
    BOS_RES_TYPE_FONT,
    BOS_RES_TYPE_THEME,
    BOS_RES_TYPE_BINARY
} BOS_ResourceType;

typedef struct {
    char            res_id[64];
    BOS_ResourceType type;
    const void*     data_ptr;
    size_t          data_size;
} BOS_Resource;

BOS_Result BOS_Resource_Load(const char* res_id, BOS_ResourceType type, BOS_Resource** out_res);
void       BOS_Resource_Free(BOS_Resource* res);

#endif /* BOS_RES_H */
