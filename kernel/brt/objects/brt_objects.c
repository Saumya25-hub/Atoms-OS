#include "../include/brt_api.h"
#include "kernel/core/lib/include/string.h"

static BRTObject s_object_pool[BRT_MAX_OBJECTS];
static uint32_t  s_next_obj_id = 1;

BRTObject* BRT_CreateObject(BRTRuntime* rt, const char* name, BRTObjectType type) {
    if (!name) return NULL;
    for (int i = 0; i < BRT_MAX_OBJECTS; i++) {
        if (s_object_pool[i].ref_count == 0) {
            memset(&s_object_pool[i], 0, sizeof(BRTObject));
            s_object_pool[i].object_id = s_next_obj_id++;
            strcpy(s_object_pool[i].name, name);
            s_object_pool[i].type = type;
            s_object_pool[i].ref_count = 1;
            s_object_pool[i].owner_pid = rt ? rt->owner_pid : 0;
            return &s_object_pool[i];
        }
    }
    return NULL;
}

void BRT_RetainObject(BRTObject* obj) {
    if (obj) obj->ref_count++;
}

void BRT_ReleaseObject(BRTObject* obj) {
    if (obj && obj->ref_count > 0) {
        obj->ref_count--;
    }
}
