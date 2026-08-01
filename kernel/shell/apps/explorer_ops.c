#include "explorer_ops.h"
#include "explorer_view.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/drivers/display/display.h"

void explorer_ops_copy(ExplorerContext* ctx, const char* src_path) {
    (void)ctx;
    (void)src_path;
}

void explorer_ops_paste(ExplorerContext* ctx, const char* dest_path) {
    (void)ctx;
    (void)dest_path;
}

void explorer_ops_delete(ExplorerContext* ctx, const char* path) {
    if (!ctx || !path) return;
    vfs_delete(path);
    explorer_cache_invalidate(ctx->current_path);
    explorer_navigate(ctx, ctx->current_path);
}

void explorer_ops_rename(ExplorerContext* ctx, const char* old_path, const char* new_path) {
    if (!ctx || !old_path || !new_path) return;
    vfs_rename(old_path, new_path);
    explorer_cache_invalidate(ctx->current_path);
    explorer_navigate(ctx, ctx->current_path);
}

void explorer_ops_new_folder(ExplorerContext* ctx, const char* path, const char* name) {
    if (!ctx || !path || !name) return;
    (void)name;
    vfs_mkdir(path);
    explorer_cache_invalidate(ctx->current_path);
    explorer_navigate(ctx, ctx->current_path);
}
