#include "explorer_ui.h"
#include "explorer_sidebar.h"
#include "kernel/shell/desktop_shell/desktop_shell.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/wm/bwe/include/bwe_layout.h"

// --- Internal Callbacks ---
static void btn_back_clicked(uint32_t btn_id) {
    // Basic back stub
}

static void btn_forward_clicked(uint32_t btn_id) {
    // Basic forward stub
}

static void btn_up_clicked(uint32_t btn_id) {
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
    
    if (strcmp(ctx->current_path, "/") == 0) return;
    
    int last_slash = -1;
    for (int i = 0; ctx->current_path[i] != '\0'; i++) {
        if (ctx->current_path[i] == '/') last_slash = i;
    }
    
    if (last_slash > 0) {
        ctx->current_path[last_slash] = '\0';
    } else if (last_slash == 0) {
        ctx->current_path[1] = '\0';
    }
    explorer_navigate(ctx, ctx->current_path);
}

static void btn_refresh_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    if (!ctx) return;
    
    explorer_navigate(ctx, ctx->current_path);
}

// --- Public API ---

int explorer_ui_init(ExplorerContext* ctx) {
    // Create Main Window (800x600)
    bwe_error_t err = BOS_CreateWindow(100, 100, 800, 600, "BOS Explorer", &ctx->window_id);
    if (err != 0) return -1;
    
    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) win->user_data = ctx;
    
    // Create Toolbar (Top, Darkish blue background)
    BOS_CreatePanel(ctx->window_id, 0, 0, 790, 40, 0xFF0F172A, &ctx->toolbar_id);
    BWE_SetAnchorMode(ctx->toolbar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_RIGHT);
    
    // Create Navigation Buttons
    uint32_t b1, b2, b3, b4;
    BOS_CreateButton(ctx->toolbar_id, 10, 5, 30, 30, "<", btn_back_clicked, &b1);
    BOS_CreateButton(ctx->toolbar_id, 45, 5, 30, 30, ">", btn_forward_clicked, &b2);
    BOS_CreateButton(ctx->toolbar_id, 80, 5, 30, 30, "^", btn_up_clicked, &b3);
    BOS_CreateButton(ctx->toolbar_id, 115, 5, 80, 30, "Refresh", btn_refresh_clicked, &b4);
    
    // Create Path Bar
    BOS_CreateTextbox(ctx->toolbar_id, 205, 5, 450, 30, "/", &ctx->pathbar_id);
    BWE_SetAnchorMode(ctx->pathbar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_RIGHT);
    
    // Create Search Bar
    uint32_t sb;
    BOS_CreateTextbox(ctx->toolbar_id, 665, 5, 115, 30, "Search...", &sb);
    BWE_SetAnchorMode(sb, BWE_ANCHOR_RIGHT | BWE_ANCHOR_TOP);
    
    // Create Status Bar (Bottom)
    BOS_CreatePanel(ctx->window_id, 0, 540, 790, 20, 0xFF1E293B, &ctx->statusbar_id);
    BWE_SetAnchorMode(ctx->statusbar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_BOTTOM | BWE_ANCHOR_RIGHT);
    BOS_CreateLabel(ctx->statusbar_id, 10, 2, "0 items", 0xFF94A3B8, &ctx->status_label_id);
    
    // Create Sidebar (Left)
    BOS_CreatePanel(ctx->window_id, 0, 40, 180, 500, 0xFF1E293B, &ctx->sidebar_id);
    BWE_SetAnchorMode(ctx->sidebar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_BOTTOM);
    explorer_sidebar_create(ctx);
    
    // Create View Panel (Right)
    BOS_CreatePanel(ctx->window_id, 180, 40, 610, 500, 0xFF0B1120, &ctx->view_panel_id);
    BWE_SetAnchorMode(ctx->view_panel_id, BWE_ANCHOR_ALL);
    
    return 0;
}

void explorer_ui_update_pathbar(ExplorerContext* ctx, const char* path) {
    if (!ctx || ctx->pathbar_id == 0) return;
    BWE_Window* tb = BWE_GetWindow(ctx->pathbar_id);
    if (tb) {
        strcpy(tb->control_data.textbox.text, path);
        BWE_InvalidateWindow(ctx->pathbar_id);
    }
}

void explorer_ui_update_status(ExplorerContext* ctx, int item_count) {
    if (!ctx || ctx->status_label_id == 0) return;
    
    char count_str[16];
    char temp[16];
    int i = 0;
    if (item_count == 0) { 
        strcpy(count_str, "0"); 
    } else {
        while (item_count > 0) {
            temp[i++] = (item_count % 10) + '0';
            item_count /= 10;
        }
        int j = 0;
        while (i > 0) count_str[j++] = temp[--i];
        count_str[j] = '\0';
    }
    
    char final_str[32];
    strcpy(final_str, count_str);
    int len = strlen(final_str);
    strcpy(final_str + len, " items");
    
    BWE_Window* lbl = BWE_GetWindow(ctx->status_label_id);
    if (lbl) {
        strcpy(lbl->control_data.label.text, final_str);
        BWE_InvalidateWindow(ctx->status_label_id);
    }
}
