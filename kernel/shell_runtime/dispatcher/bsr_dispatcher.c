#include "../include/bsr_api.h"
#include "kernel/botree/include/botree.h"
#include "kernel/botree/runtime/include/dre_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BSR_Open(BSR_Runtime* rt, const char* target_path) {
    if (!rt || !rt->active || !target_path) return -1;

    if (rt->directory) {
        if (BDeRuntime_Open(rt->directory, target_path) == 0) {
            strcpy(rt->current_path, BDeRuntime_GetCurrentDirectory(rt->directory));
            BSR_AddRecent(rt->current_path);
            return 0;
        }
    }
    return -1;
}

int32_t BSR_Back(BSR_Runtime* rt) {
    if (!rt || !rt->active || !rt->directory) return -1;
    if (BDeRuntime_Back(rt->directory) == 0) {
        strcpy(rt->current_path, BDeRuntime_GetCurrentDirectory(rt->directory));
        return 0;
    }
    return -1;
}

int32_t BSR_Forward(BSR_Runtime* rt) {
    if (!rt || !rt->active || !rt->directory) return -1;
    if (BDeRuntime_Forward(rt->directory) == 0) {
        strcpy(rt->current_path, BDeRuntime_GetCurrentDirectory(rt->directory));
        return 0;
    }
    return -1;
}

int32_t BSR_Up(BSR_Runtime* rt) {
    if (!rt || !rt->active || !rt->directory) return -1;
    if (BDeRuntime_Up(rt->directory) == 0) {
        strcpy(rt->current_path, BDeRuntime_GetCurrentDirectory(rt->directory));
        return 0;
    }
    return -1;
}

int32_t BSR_Copy(BSR_Runtime* rt) {
    if (!rt || !rt->active) return -1;
    rt->clipboard_active = true;
    return 0;
}

int32_t BSR_Cut(BSR_Runtime* rt) {
    if (!rt || !rt->active) return -1;
    rt->clipboard_active = true;
    return 0;
}

int32_t BSR_Paste(BSR_Runtime* rt, const char* target_dir) {
    if (!rt || !rt->active || !target_dir) return -1;
    return BDe_ClipboardPaste(target_dir);
}

int32_t BSR_Delete(BSR_Runtime* rt, bool send_to_recycle) {
    if (!rt || !rt->active || !rt->directory) return -1;
    BDeTxHandle tx = BDeRuntime_DeleteSelected(rt->directory, send_to_recycle);
    return (tx != 0) ? 0 : -1;
}

int32_t BSR_Rename(BSR_Runtime* rt, const char* old_name, const char* new_name) {
    if (!rt || !rt->active || !old_name || !new_name) return -1;
    rt->rename_active = true;
    return 0;
}

int32_t BSR_NewFolder(BSR_Runtime* rt, const char* folder_name) {
    if (!rt || !rt->active || !folder_name) return -1;
    char full_path[BDE_PATH_MAX];
    BDe_PathJoin(rt->current_path, folder_name, full_path, BDE_PATH_MAX);
    extern int vfs_mkdir(const char*);
    return vfs_mkdir(full_path);
}
