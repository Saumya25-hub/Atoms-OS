#include "../include/bsom_api.h"
#include "kernel/core/lib/include/string.h"

BSOMObject* BSOM_CreateShortcut(const char* target_path, const char* shortcut_path) {
    if (!target_path || !shortcut_path) return NULL;
    BSOMObject* obj = BSOM_CreateObject(shortcut_path, BSOM_CLASS_SHORTCUT);
    if (obj) {
        BSOM_SetProperty(obj, "Target", target_path);
    }
    return obj;
}

int32_t BSOM_ResolveShortcut(BSOMObject* shortcut_obj, char* out_target_path) {
    if (!shortcut_obj || !out_target_path) return -1;
    return BSOM_GetProperty(shortcut_obj, "Target", out_target_path, BDE_PATH_MAX);
}
