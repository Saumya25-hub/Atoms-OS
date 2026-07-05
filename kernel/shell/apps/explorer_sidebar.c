#include "explorer_sidebar.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/lib/include/string.h"

static void sidebar_btn_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    // Find parent window to get context
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    if (!ctx) return;
    
    // Determine path based on button text
    const char* text = btn->control_data.button.text;
    if (strcmp(text, "This PC") == 0) explorer_navigate(ctx, "/");
    else if (strcmp(text, "Desktop") == 0) explorer_navigate(ctx, "/DESKTOP");
    else if (strcmp(text, "Documents") == 0) explorer_navigate(ctx, "/DOCS");
    else if (strcmp(text, "Downloads") == 0) explorer_navigate(ctx, "/DOWNLOAD");
    else if (strcmp(text, "Music") == 0) explorer_navigate(ctx, "/MUSIC");
    else if (strcmp(text, "Pictures") == 0) explorer_navigate(ctx, "/PHOTO");
    else if (strcmp(text, "Videos") == 0) explorer_navigate(ctx, "/VIDEO");
}

void explorer_sidebar_create(ExplorerContext* ctx) {
    if (!ctx || ctx->sidebar_id == 0) return;
    
    uint32_t y = 10;
    uint32_t tmp;
    
    // Simple flat button look
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "This PC", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Desktop", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Documents", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Downloads", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Music", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Pictures", sidebar_btn_clicked, &tmp); y += 35;
    BOS_CreateButton(ctx->sidebar_id, 10, y, 160, 30, "Videos", sidebar_btn_clicked, &tmp);
    
    // Style buttons to blend into sidebar
    // Left as default BOS button style for now, but they can be customized via BWE.
}
