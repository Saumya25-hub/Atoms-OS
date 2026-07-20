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

// --- Public View Render ---
void explorer_view_render(ExplorerContext* ctx) {
    if (!ctx || ctx->view_panel_id == 0) return;
    
    // Destroy the old view panel to clear contents
    uint32_t parent_win = ctx->window_id;
    BOS_DestroySurface(ctx->view_panel_id);
    
    // Recreate it (matches explorer_ui.c layout: x=180, y=40, w=610, h=500)
    BOS_CreatePanel(parent_win, 180, 40, 610, 500, 0xFF0B1120, &ctx->view_panel_id);
    
    int index = 0;
    vfs_dirent_t entry;
    
    uint32_t x_offset = 15;
    uint32_t y_offset = 15;
    int item_count = 0;
    
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
            if (x_offset > 500) {
                x_offset = 15;
                y_offset += 120;
            }
            item_count++;
        }
        index++;
    }
    
    explorer_ui_update_status(ctx, item_count);
}
