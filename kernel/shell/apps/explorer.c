#include "explorer.h"
#include "explorer_ui.h"
#include "explorer_view.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

int explorer_init(uint32_t* out_win) {
    extern void* kcalloc(size_t num, size_t size);
    ExplorerContext* ctx = (ExplorerContext*)kcalloc(1, sizeof(ExplorerContext));
    if (!ctx) return -1;
    
    // Default mode
    ctx->view_mode = EXP_VIEW_ICON;
    strcpy(ctx->current_path, "/");
    
    // Initialize UI
    if (explorer_ui_init(ctx) != 0) {
        // Free ctx? In OS it might just leak if window creation fails, but lets be clean.
        extern void kfree(void*);
        kfree(ctx);
        return -1;
    }
    
    // Load initial directory
    explorer_navigate(ctx, "/");
    
    if (out_win) {
        *out_win = ctx->window_id;
    }
    
    return 0; // Success
}

void explorer_navigate(ExplorerContext* ctx, const char* path) {
    if (!ctx) return;
    
    // 1. Update Path State
    strcpy(ctx->current_path, path);
    
    // 2. Update Toolbar Pathbar
    explorer_ui_update_pathbar(ctx, path);
    
    // 3. Render View Panel
    explorer_view_render(ctx);
}

void explorer_set_view_mode(ExplorerContext* ctx, ExplorerViewMode mode) {
    if (!ctx) return;
    ctx->view_mode = mode;
    explorer_view_render(ctx);
}
