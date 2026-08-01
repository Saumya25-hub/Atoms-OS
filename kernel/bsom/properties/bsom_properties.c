#include "../include/bsom_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BSOM_GetProperty(BSOMObject* obj, const char* key, char* out_val, size_t max_len) {
    if (!obj || !key || !out_val || max_len == 0) return -1;
    for (uint32_t i = 0; i < obj->prop_count; i++) {
        if (strcmp(obj->properties[i].key, key) == 0) {
            strncpy(out_val, obj->properties[i].value, max_len - 1);
            out_val[max_len - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

int32_t BSOM_SetProperty(BSOMObject* obj, const char* key, const char* val) {
    if (!obj || !key || !val) return -1;
    if (obj->prop_count < BSOM_MAX_PROPERTIES) {
        strcpy(obj->properties[obj->prop_count].key, key);
        strcpy(obj->properties[obj->prop_count].value, val);
        obj->prop_count++;
        return 0;
    }
    return -1;
}

int32_t BSOM_ShowProperties(BSOMObject* obj) {
    if (!obj) return -1;
    return 0;
}

int32_t BSOM_GetContextMenu(BSOMObject* obj, BSOMContextMenu* out_menu) {
    if (!obj || !out_menu) return -1;
    memset(out_menu, 0, sizeof(BSOMContextMenu));
    out_menu->item_count = 3;
    out_menu->items[0].command_id = 1; strcpy(out_menu->items[0].label, "Open"); out_menu->items[0].enabled = true;
    out_menu->items[1].command_id = 2; strcpy(out_menu->items[1].label, "Copy"); out_menu->items[1].enabled = true;
    out_menu->items[2].command_id = 3; strcpy(out_menu->items[2].label, "Delete"); out_menu->items[2].enabled = true;
    return 0;
}

int32_t BSOM_Invoke(BSOMObject* obj) {
    if (!obj) return -1;
    return 0;
}

int32_t BSOM_GetChildren(BSOMObject* obj, BSOMObject*** out_children, uint32_t* out_count) {
    if (!obj || !out_children || !out_count) return -1;
    *out_children = NULL;
    *out_count = 0;
    return 0;
}

int32_t BSOM_Enumerate(BSOMObject* obj, BSOMEnumCallback cb) {
    if (!obj || !cb) return -1;
    cb(obj, NULL);
    return 0;
}

int32_t BSOM_Copy(BSOMObject* obj, BSOMObject* dest_folder) {
    if (!obj || !dest_folder) return -1;
    return 0;
}

int32_t BSOM_Move(BSOMObject* obj, BSOMObject* dest_folder) {
    if (!obj || !dest_folder) return -1;
    return 0;
}

int32_t BSOM_Delete(BSOMObject* obj, bool send_to_recycle) {
    if (!obj) return -1;
    (void)send_to_recycle;
    return 0;
}

int32_t BSOM_Rename(BSOMObject* obj, const char* new_name) {
    if (!obj || !new_name) return -1;
    strcpy(obj->name, new_name);
    return 0;
}
