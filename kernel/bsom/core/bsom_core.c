#include "../include/bsom_api.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static BSOMObject s_object_pool[BSOM_MAX_OBJECTS];
static uint32_t   s_next_obj_id = 1;
static bool       s_bsom_inited = false;

int32_t BSOM_Init(void) {
    if (s_bsom_inited) return 0;
    display_print("[BSOM] Initializing BOS Shell Object Manager V1.0...\n");
    memset(s_object_pool, 0, sizeof(s_object_pool));
    s_bsom_inited = true;
    return 0;
}

BSOMObject* BSOM_CreateObject(const char* name, BSOMClassType class_type) {
    if (!name) return NULL;
    for (int i = 0; i < BSOM_MAX_OBJECTS; i++) {
        if (s_object_pool[i].ref_count == 0) {
            memset(&s_object_pool[i], 0, sizeof(BSOMObject));
            s_object_pool[i].object_id = s_next_obj_id++;
            strcpy(s_object_pool[i].name, name);
            strcpy(s_object_pool[i].path, name);
            s_object_pool[i].class_type = class_type;
            s_object_pool[i].ref_count = 1;
            s_object_pool[i].icon_id = 1;
            return &s_object_pool[i];
        }
    }
    return NULL;
}

void BSOM_Retain(BSOMObject* obj) {
    if (obj) obj->ref_count++;
}

void BSOM_Release(BSOMObject* obj) {
    if (obj && obj->ref_count > 0) {
        obj->ref_count--;
    }
}
