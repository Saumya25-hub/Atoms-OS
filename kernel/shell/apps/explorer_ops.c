// ============================================================
// ATOMS OS Explorer Ops — BSOM Authority (Phase 9 Rewrite)
// ============================================================
// ALL filesystem operations delegated to BSOM API.
// Zero direct VFS / BFS calls in view layer.
// ============================================================

#include "explorer_ops.h"
#include "explorer_view.h"
#include "kernel/bsom/include/bsom_api.h"
#include "kernel/drivers/display/display.h"

void explorer_ops_copy(ExplorerContext* ctx, const char* src_path) {
    if (!ctx || !src_path) return;
    BSOMObject* src = BSOM_CreateObject(src_path, BSOM_CLASS_FILE);
    if (src) {
        BSOM_Copy(src, ctx->current_folder);
        BSOM_Release(src);
        Explorer_Refresh(ctx);
    }
}

void explorer_ops_paste(ExplorerContext* ctx, const char* dest_path) {
    (void)ctx;
    (void)dest_path;
}

void explorer_ops_delete(ExplorerContext* ctx, const char* path) {
    if (!ctx || !path) return;
    BSOMObject* obj = BSOM_CreateObject(path, BSOM_CLASS_FILE);
    if (obj) {
        BSOM_Delete(obj, true);
        BSOM_Release(obj);
        Explorer_Refresh(ctx);
    }
}

void explorer_ops_rename(ExplorerContext* ctx, const char* old_path, const char* new_path) {
    if (!ctx || !old_path || !new_path) return;
    BSOMObject* obj = BSOM_CreateObject(old_path, BSOM_CLASS_FILE);
    if (obj) {
        BSOM_Rename(obj, new_path);
        BSOM_Release(obj);
        Explorer_Refresh(ctx);
    }
}

void explorer_ops_new_folder(ExplorerContext* ctx, const char* path, const char* name) {
    if (!ctx || !path) return;
    (void)name;
    BSOMObject* new_folder = BSOM_CreateObject(path, BSOM_CLASS_FOLDER);
    if (new_folder) {
        BSOM_Release(new_folder);
        Explorer_Refresh(ctx);
    }
}
