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

#include "kernel/wm/botheme/botheme.h"

// --- Public API ---

int explorer_ui_init(ExplorerContext* ctx) {
    // Create Main Window (800x600)
    bwe_error_t err = BOS_CreateWindow(100, 100, 800, 600, "BOS Explorer", &ctx->window_id);
    if (err != 0) return -1;
    
    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) win->user_data = ctx;
    
    // 1. Create Root DockPanel spanning entire client area
    uint32_t root_dock = 0;
    BOS_CreateDockPanel(ctx->window_id, &root_dock);
    BWE_SetDockPosition(root_dock, BWE_DOCK_FILL);
    BWE_Window* root_win = BWE_GetWindow(root_dock);
    if (root_win) {
        root_win->local_bounds = (BWE_Rect){0, 0, 790, 560};
        root_win->control_data.panel.bg_color = BOTHEME_GetColor(BOTHEME_SURFACE_PRIMARY);
    }

    // 2. Create Toolbar (DockTop, Height 40px)
    BOS_CreateDockPanel(root_dock, &ctx->toolbar_id);
    BWE_SetDockPosition(ctx->toolbar_id, BWE_DOCK_TOP);
    BWE_Window* tb_win = BWE_GetWindow(ctx->toolbar_id);
    if (tb_win) {
        tb_win->local_bounds = (BWE_Rect){0, 0, 790, 40};
        tb_win->layout_props.desired_size.height = 40;
        tb_win->control_data.panel.bg_color = BOTHEME_GetColor(BOTHEME_SURFACE_TERTIARY);
    }
    
    // Create Navigation Buttons inside Toolbar
    uint32_t b1, b2, b3, b4;
    BOS_CreateButton(ctx->toolbar_id, 10, 5, 30, 30, "<", btn_back_clicked, &b1);
    BOS_CreateButton(ctx->toolbar_id, 45, 5, 30, 30, ">", btn_forward_clicked, &b2);
    BOS_CreateButton(ctx->toolbar_id, 80, 5, 30, 30, "^", btn_up_clicked, &b3);
    BOS_CreateButton(ctx->toolbar_id, 115, 5, 70, 30, "Refresh", btn_refresh_clicked, &b4);
    
    // Create Path Bar inside Toolbar
    BOS_CreateTextbox(ctx->toolbar_id, 195, 5, 450, 30, "/", &ctx->pathbar_id);
    BWE_SetAnchorMode(ctx->pathbar_id, BWE_ANCHOR_LEFT | BWE_ANCHOR_TOP | BWE_ANCHOR_RIGHT);
    
    // Create Search Bar inside Toolbar
    uint32_t sb;
    BOS_CreateTextbox(ctx->toolbar_id, 655, 5, 115, 30, "Search...", &sb);
    BWE_SetAnchorMode(sb, BWE_ANCHOR_RIGHT | BWE_ANCHOR_TOP);
    
    // 3. Create Status Bar (DockBottom, Height 24px)
    BOS_CreateDockPanel(root_dock, &ctx->statusbar_id);
    BWE_SetDockPosition(ctx->statusbar_id, BWE_DOCK_BOTTOM);
    BWE_Window* sb_win = BWE_GetWindow(ctx->statusbar_id);
    if (sb_win) {
        sb_win->local_bounds = (BWE_Rect){0, 0, 790, 24};
        sb_win->layout_props.desired_size.height = 24;
        sb_win->control_data.panel.bg_color = BOTHEME_GetColor(BOTHEME_SURFACE_SECONDARY);
    }
    BOS_CreateLabel(ctx->statusbar_id, 10, 4, "0 items", BOTHEME_GetColor(BOTHEME_TEXT_SECONDARY), &ctx->status_label_id);
    
    // 4. Create Sidebar (DockLeft, Width 180px)
    BOS_CreateDockPanel(root_dock, &ctx->sidebar_id);
    BWE_SetDockPosition(ctx->sidebar_id, BWE_DOCK_LEFT);
    BWE_Window* sb_panel = BWE_GetWindow(ctx->sidebar_id);
    if (sb_panel) {
        sb_panel->local_bounds = (BWE_Rect){0, 0, 180, 500};
        sb_panel->layout_props.desired_size.width = 180;
        sb_panel->control_data.panel.bg_color = BOTHEME_GetColor(BOTHEME_SURFACE_SECONDARY);
    }
    explorer_sidebar_create(ctx);

    // 5. Create Main File View Panel (DockFill)
    BOS_CreateDockPanel(root_dock, &ctx->view_panel_id);
    BWE_SetDockPosition(ctx->view_panel_id, BWE_DOCK_FILL);
    BWE_Window* view_p = BWE_GetWindow(ctx->view_panel_id);
    if (view_p) {
        view_p->local_bounds = (BWE_Rect){0, 0, 610, 500};
        view_p->control_data.panel.bg_color = BOTHEME_GetColor(BOTHEME_SURFACE_PRIMARY);
    }

    // Run Two-Pass Layout Engine to arrange entire Explorer Window
    BWE_UpdateLayout(ctx->window_id);

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
