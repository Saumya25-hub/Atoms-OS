#include "explorer_view.h"
#include "explorer_ui.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

// --- Internal Helpers ---
static void exp_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static void file_item_clicked(uint32_t btn_id) {
    BWE_Window* btn = BWE_GetWindow(btn_id);
    if (!btn) return;
    
    BWE_Window* parent = BWE_GetWindow(btn->parent_id);
    while (parent && parent->parent_id != BWE_DESKTOP_ID && parent->parent_id != parent->id) {
        parent = BWE_GetWindow(parent->parent_id);
    }
    if (!parent) return;
    ExplorerContext* ctx = (ExplorerContext*)parent->user_data;
    if (!ctx) return;
    
    const char* filename = btn->control_data.button.text;
    uint32_t bg = btn->control_data.button.bg_color;
    
    char full_path[256];
    strcpy(full_path, ctx->current_path);
    int len = strlen(full_path);
    if (full_path[len - 1] != '/') {
        full_path[len] = '/';
        full_path[len + 1] = '\0';
    }
    exp_strcat(full_path, filename);
    
    if (bg == 0xFFE0F2FE || bg == 0xFFDBEAFE) {
        // It's a directory
        explorer_navigate(ctx, full_path);
    } else {
        // Launch file
        char msg[256];
        strcpy(msg, "Open file: ");
        exp_strcat(msg, filename);
        extern void Shell_ShowNotification(const char* title, const char* msg, uint32_t duration_ms);
        Shell_ShowNotification("File Manager", msg, 4000);
    }
}

#include "kernel/wm/botheme/botheme.h"

// --- Public View Render ---
void explorer_view_render(ExplorerContext* ctx) {
    if (!ctx || ctx->view_panel_id == 0) return;
    
    BWE_Window* view_p = BWE_GetWindow(ctx->view_panel_id);
    if (!view_p) return;

    // Clear previous file icon buttons without destroying the view_panel container
    while (view_p->child_count > 0) {
        uint32_t child_id = view_p->children[view_p->child_count - 1];
        BOS_DestroySurface(child_id);
    }
    
    int index = 0;
    vfs_dirent_t entry;
    
    uint32_t x_offset = 15;
    uint32_t y_offset = 15;
    int item_count = 0;

    int32_t panel_w = view_p->screen_bounds.width;
    if (panel_w <= 120) panel_w = 600;
    
    while (vfs_readdir(ctx->current_path, index, &entry) == 0) {
        if (strlen(entry.name) > 0) {
            uint32_t item_id;
            uint32_t bg_color = entry.is_directory ? 0xFFDBEAFE : 0xFF1E293B; // Light blue folder, dark file
            uint32_t text_color = entry.is_directory ? 0xFF0F172A : 0xFFF8FAFC;
            
            char display_name[64];
            strcpy(display_name, entry.name);
            
            BOS_CreateButton(ctx->view_panel_id, x_offset, y_offset, 100, 100, display_name, file_item_clicked, &item_id);
            BWE_Window* btn = BWE_GetWindow(item_id);
            if (btn) {
                btn->control_data.button.bg_color = bg_color;
                btn->control_data.button.text_color = text_color;
            }
            
            x_offset += 120;
            if (x_offset + 100 > (uint32_t)panel_w) {
                x_offset = 15;
                y_offset += 120;
            }
            item_count++;
        }
        index++;
    }
    
    explorer_ui_update_status(ctx, item_count);

    // Run arrange pass so new file buttons get valid screen_bounds
    BWE_UpdateLayout(ctx->window_id);
    BWE_InvalidateWindow(ctx->window_id);
}
