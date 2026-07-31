#ifndef BOS_UI_REFLECTION_H
#define BOS_UI_REFLECTION_H

#include "platform/include/bos_types.h"

typedef enum {
    BOS_PROP_INT = 0,
    BOS_PROP_FLOAT,
    BOS_PROP_STRING,
    BOS_PROP_COLOR,
    BOS_PROP_BOOL
} BOS_PropertyType;

typedef struct {
    const char*      name;
    const char*      category;
    BOS_PropertyType type;
    size_t           struct_offset;
} BOS_PropertyDesc;

typedef struct {
    const char* name;
    size_t      callback_offset;
} BOS_EventDesc;

typedef struct {
    const char*             class_name;
    const BOS_PropertyDesc* properties;
    uint32_t                property_count;
    const BOS_EventDesc*    events;
    uint32_t                event_count;
} BOS_ControlMetadata;

void BOS_Reflection_RegisterControl(const BOS_ControlMetadata* meta);
const BOS_ControlMetadata* BOS_Reflection_FindControl(const char* class_name);

#endif /* BOS_UI_REFLECTION_H */
