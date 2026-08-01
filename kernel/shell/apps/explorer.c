#include "explorer.h"
#include "explorer_ui.h"
#include "explorer_view.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

int explorer_init(uint32_t* out_win) {
    static ExplorerContext static_ctx;
    memset(&static_ctx, 0, sizeof(ExplorerContext));
    
    explorer_cache_init();
    explorer_profiler_init(&static_ctx.profiler);
    
    static_ctx.view_mode = EXP_VIEW_ICON;
    static_ctx.selected_index = -1;
    static_ctx.hovered_index = -1;
    strcpy(static_ctx.current_path, "/");
    
    if (explorer_ui_init(&static_ctx) != 0) {
        return -1;
    }
    
    explorer_navigate(&static_ctx, "/");
    
    if (out_win) {
        *out_win = static_ctx.window_id;
    }
    return 0;
}

void explorer_navigate(ExplorerContext* ctx, const char* path) {
    if (!ctx || !path) return;
    
    strcpy(ctx->current_path, path);
    ctx->scroll_offset_y = 0;
    ctx->selected_index = -1;
    ctx->hovered_index = -1;
    
    explorer_ui_update_pathbar(ctx, path);
    explorer_view_render(ctx);
}

void explorer_set_view_mode(ExplorerContext* ctx, ExplorerViewMode mode) {
    if (!ctx) return;
    ctx->view_mode = mode;
    explorer_view_render(ctx);
}
