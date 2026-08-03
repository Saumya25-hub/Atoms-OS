#include "explorer_ui.h"
#include "explorer_sidebar.h"
#include "explorer_view.h"
#include "kernel/shell/desktop_shell/desktop_shell.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/wm/botheme/botheme.h"

// --- Internal Callbacks ---
static void btn_up_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
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
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
    explorer_cache_invalidate(ctx->current_path);
    explorer_navigate(ctx, ctx->current_path);
}

#include "kernel/vfs/vfs_legacy/include/vfs.h"

static void btn_new_folder_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
    const char* cur_path = (ctx->current_folder && ctx->current_folder->path) ? ctx->current_folder->path : "/";
    if (!cur_path || strlen(cur_path) == 0) cur_path = "/";

    char new_path[256];
    if (strcmp(cur_path, "/") == 0) {
        strcpy(new_path, "/New Folder");
    } else {
        strcpy(new_path, cur_path);
        strcat(new_path, "/New Folder");
    }
    vfs_mkdir(new_path);
    BSOMObject* nf = BSOM_CreateObject(new_path, BSOM_CLASS_FOLDER);
    if (nf) BSOM_Release(nf);
    Explorer_Refresh(ctx);
}

static void btn_delete_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    
    if (ctx->selected_index >= 0 && ctx->selected_index < (int32_t)ctx->view_item_count) {
        BSOMObject* obj = ctx->view_items[ctx->selected_index].obj;
        if (obj && strlen(obj->path) > 0) {
            vfs_delete(obj->path);
            BSOM_Delete(obj, false);
            Explorer_Refresh(ctx);
        }
    }
}

static void btn_view_icon_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) parent = BWE_GetWindow(parent->parent_id);
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    explorer_set_view_mode(ctx, EXP_VIEW_ICON);
}

static void btn_view_list_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) parent = BWE_GetWindow(parent->parent_id);
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    explorer_set_view_mode(ctx, EXP_VIEW_LIST);
}

static void scrollbar_scrolled(uint32_t scr_id, int32_t val) {
    BWE_Window* scr = BWE_GetWindow(scr_id);
    if (!scr) return;
    BWE_Window* parent = BWE_GetWindow(scr->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) parent = BWE_GetWindow(parent->parent_id);
    if (!parent || !parent->user_data) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    ctx->scroll_offset_y = val;
    BWE_InvalidateWindow(ctx->window_id);
}

int explorer_ui_init(ExplorerContext* ctx) {
    if (!ctx) return -1;
    
    // Create Modern Main Window (840x620)
    bwe_error_t err = BOS_CreateWindow(80, 50, 840, 620, "File Explorer", &ctx->window_id);
    if (err != 0) return -1;
    
    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) win->user_data = ctx;
    
    // 1. Root DockPanel
    uint32_t root_dock = 0;
    BOS_CreateDockPanel(ctx->window_id, &root_dock);
    BWE_SetDockPosition(root_dock, BWE_DOCK_FILL);
    BWE_Window* root_win = BWE_GetWindow(root_dock);
    if (root_win) {
        root_win->local_bounds = (BWE_Rect){0, 0, 830, 580};
        root_win->control_data.panel.bg_color = 0xFF0F172A; // Modern Dark Slate
    }

    // 2. Navigation Header & Toolbar (DockTop, Height 72px)
    BOS_CreateDockPanel(root_dock, &ctx->toolbar_id);
    BWE_SetDockPosition(ctx->toolbar_id, BWE_DOCK_TOP);
    BWE_Window* tb_win = BWE_GetWindow(ctx->toolbar_id);
    if (tb_win) {
        tb_win->local_bounds = (BWE_Rect){0, 0, 830, 72};
        tb_win->control_data.panel.bg_color = 0xFF1E293B; // Dark Slate acrylic
    }
    
    // Navigation Buttons (Row 1)
    uint32_t b1, b2, b3, b4, b5, b6, b7;
    BOS_CreateButton(ctx->toolbar_id, 6, 6, 28, 28, "<", NULL, &b1);
    BOS_CreateButton(ctx->toolbar_id, 38, 6, 28, 28, ">", NULL, &b2);
    BOS_CreateButton(ctx->toolbar_id, 70, 6, 28, 28, "^", btn_up_clicked, &b3);
    BOS_CreateButton(ctx->toolbar_id, 102, 6, 32, 28, "R", btn_refresh_clicked, &b4);
    
    // Breadcrumb Address Bar
    BOS_CreateTextbox(ctx->toolbar_id, 140, 6, 470, 28, " > Home > Documents", &ctx->pathbar_id);
    
    // Search Box
    uint32_t search_id;
    BOS_CreateTextbox(ctx->toolbar_id, 618, 6, 200, 28, "Type here to search...", &search_id);

    // Command Bar (Row 2 - Action Icons)
    BOS_CreateButton(ctx->toolbar_id, 6, 40, 75, 26, "+ New v", btn_new_folder_clicked, &b5);
    BOS_CreateButton(ctx->toolbar_id, 86, 40, 50, 26, "Cut", NULL, &b6);
    BOS_CreateButton(ctx->toolbar_id, 140, 40, 55, 26, "Copy", NULL, &b7);
    
    uint32_t b8, b9, b10, b11, b12;
    BOS_CreateButton(ctx->toolbar_id, 200, 40, 55, 26, "Paste", NULL, &b8);
    BOS_CreateButton(ctx->toolbar_id, 260, 40, 65, 26, "Rename", NULL, &b9);
    BOS_CreateButton(ctx->toolbar_id, 330, 40, 60, 26, "Delete", btn_delete_clicked, &b10);
    
    // View Switchers (Row 2 Right)
    BOS_CreateButton(ctx->toolbar_id, 700, 40, 55, 26, "Grid", btn_view_icon_clicked, &b11);
    BOS_CreateButton(ctx->toolbar_id, 760, 40, 55, 26, "List", btn_view_list_clicked, &b12);

    // 3. Status Bar (DockBottom, Height 26px)
    BOS_CreateDockPanel(root_dock, &ctx->statusbar_id);
    BWE_SetDockPosition(ctx->statusbar_id, BWE_DOCK_BOTTOM);
    BWE_Window* sb_win = BWE_GetWindow(ctx->statusbar_id);
    if (sb_win) {
        sb_win->local_bounds = (BWE_Rect){0, 0, 830, 26};
        sb_win->control_data.panel.bg_color = 0xFF1E293B;
    }
    BOS_CreateLabel(ctx->statusbar_id, 12, 5, "24 items | 1 item selected", 0xFF94A3B8, &ctx->status_label_id);

    // 4. Sidebar Navigation Pane (DockLeft, Width 180px)
    BOS_CreateDockPanel(root_dock, &ctx->sidebar_id);
    BWE_SetDockPosition(ctx->sidebar_id, BWE_DOCK_LEFT);
    BWE_Window* sb_panel = BWE_GetWindow(ctx->sidebar_id);
    if (sb_panel) {
        sb_panel->local_bounds = (BWE_Rect){0, 0, 180, 482};
        sb_panel->control_data.panel.bg_color = 0xFF0F172A;
    }
    explorer_sidebar_create(ctx);

    // 5. Main Viewport Panel (DockFill)
    BOS_CreateDockPanel(root_dock, &ctx->view_panel_id);
    BWE_SetDockPosition(ctx->view_panel_id, BWE_DOCK_FILL);
    BWE_Window* view_p = BWE_GetWindow(ctx->view_panel_id);
    if (view_p) {
        view_p->local_bounds = (BWE_Rect){0, 0, 650, 482};
        view_p->control_data.panel.bg_color = 0xFF0F172A;
        view_p->user_data = ctx;
    }
    
    // Create Single Canvas Control inside View Panel
    BOS_CreateCanvas(ctx->view_panel_id, 0, 0, 630, 482, explorer_view_paint, &ctx->canvas_id);
    BWE_Window* cv_win = BWE_GetWindow(ctx->canvas_id);
    if (cv_win) cv_win->user_data = ctx;
    
    // Set user_data on remaining container panels
    if (root_win) root_win->user_data = ctx;
    if (tb_win) tb_win->user_data = ctx;
    if (sb_win) sb_win->user_data = ctx;
    if (sb_panel) sb_panel->user_data = ctx;
    
    // Create Vertical Scrollbar
    BOS_CreateScrollBar(ctx->view_panel_id, 632, 0, 16, 482, true, 0, 500, scrollbar_scrolled, &ctx->scrollbar_id);

    // Force layout passes to establish dock screen bounds
    extern void BWE_UpdateLayout(uint32_t);
    BWE_UpdateLayout(ctx->window_id);
    BWE_UpdateLayout(ctx->window_id);

    return 0;
}

void explorer_ui_update_pathbar(ExplorerContext* ctx, const char* path) {
    if (!ctx || !path) return;
    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) {
        char title_buf[256];
        strcpy(title_buf, "File Explorer - ");
        strcat(title_buf, path);
        strcpy(win->title, title_buf);
    }
    
    BWE_Window* pb = BWE_GetWindow(ctx->pathbar_id);
    if (pb) {
        char format_path[256];
        strcpy(format_path, " > Home");
        if (strcmp(path, "/") != 0) {
            strcat(format_path, " > ");
            strcat(format_path, path);
        }
        strcpy(pb->control_data.textbox.text, format_path);
        BWE_InvalidateWindow(ctx->pathbar_id);
    }

    uint32_t count = ctx ? ctx->view_item_count : 0;
    BWE_Window* lbl = BWE_GetWindow(ctx->status_label_id);
    if (lbl) {
        char stat_buf[64];
        char num_str[16];
        uint32_t v = count;
        uint32_t pos = 0;
        if (v == 0) {
            num_str[pos++] = '0';
        } else {
            char tmp[16];
            uint32_t tp = 0;
            while (v > 0) {
                tmp[tp++] = '0' + (v % 10);
                v /= 10;
            }
            while (tp > 0) {
                num_str[pos++] = tmp[--tp];
            }
        }
        num_str[pos] = '\0';

        strcpy(stat_buf, num_str);
        strcat(stat_buf, " items");
        if (ctx->selected_index >= 0 && ctx->selected_index < (int32_t)count) {
            strcat(stat_buf, " | 1 item selected");
        }
        strcpy(lbl->control_data.label.text, stat_buf);
        BWE_InvalidateWindow(ctx->status_label_id);
    }
}
