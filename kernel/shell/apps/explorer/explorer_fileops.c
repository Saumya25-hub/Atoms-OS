// ============================================================
// ATOMS OS Explorer FileOps — BSOM Authority (Phase 9 Rewrite)
// ============================================================
// ALL operations delegated to BSOM API.
// Zero direct VFS / BFS calls in application layer.
// ============================================================

#include "explorer_fileops.h"
#include "kernel/bsom/include/bsom_api.h"
#include "kernel/core/lib/include/string.h"

int explorer_fileops_new_folder(const char* target_dir, const char* name) {
    if (!target_dir || !name) return -1;
    char full_path[256];
    strcpy(full_path, target_dir);
    int len = strlen(full_path);
    if (len > 0 && full_path[len - 1] != '/') strcat(full_path, "/");
    strcat(full_path, name);

    BSOMObject* obj = BSOM_CreateObject(full_path, BSOM_CLASS_FOLDER);
    if (!obj) return -1;
    BSOM_Release(obj);
    return 0;
}

int explorer_fileops_rename(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return -1;
    BSOMObject* obj = BSOM_CreateObject(old_path, BSOM_CLASS_FILE);
    if (!obj) return -1;
    int32_t ret = BSOM_Rename(obj, new_path);
    BSOM_Release(obj);
    return ret;
}

int explorer_fileops_delete(const char* path) {
    if (!path) return -1;
    BSOMObject* obj = BSOM_CreateObject(path, BSOM_CLASS_FILE);
    if (!obj) return -1;
    int32_t ret = BSOM_Delete(obj, true);
    BSOM_Release(obj);
    return ret;
}

int explorer_fileops_copy_file(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return -1;
    BSOMObject* src = BSOM_CreateObject(src_path, BSOM_CLASS_FILE);
    BSOMObject* dest = BSOM_CreateObject(dest_dir, BSOM_CLASS_FOLDER);
    if (!src || !dest) {
        if (src) BSOM_Release(src);
        if (dest) BSOM_Release(dest);
        return -1;
    }
    int32_t ret = BSOM_Copy(src, dest);
    BSOM_Release(src);
    BSOM_Release(dest);
    return ret;
}

int explorer_fileops_move_file(const char* src_path, const char* dest_dir) {
    if (!src_path || !dest_dir) return -1;
    BSOMObject* src = BSOM_CreateObject(src_path, BSOM_CLASS_FILE);
    BSOMObject* dest = BSOM_CreateObject(dest_dir, BSOM_CLASS_FOLDER);
    if (!src || !dest) {
        if (src) BSOM_Release(src);
        if (dest) BSOM_Release(dest);
        return -1;
    }
    int32_t ret = BSOM_Move(src, dest);
    BSOM_Release(src);
    BSOM_Release(dest);
    return ret;
}

int explorer_fileops_paste_clipboard(const ExplorerClipboard* cb, const char* dest_dir) {
    if (!cb || !dest_dir || cb->count == 0) return -1;
    int success_count = 0;
    for (uint32_t i = 0; i < cb->count; i++) {
        if (cb->op == CLIPBOARD_OP_COPY) {
            if (explorer_fileops_copy_file(cb->paths[i], dest_dir) == 0) success_count++;
        } else if (cb->op == CLIPBOARD_OP_CUT) {
            if (explorer_fileops_move_file(cb->paths[i], dest_dir) == 0) success_count++;
        }
    }
    return (success_count == (int)cb->count) ? 0 : -1;
}
