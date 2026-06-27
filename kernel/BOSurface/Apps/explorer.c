#include "explorer.h"
#include "kernel/BOSurface/Core/app_manager.h"
#include "kernel/BOSurface/Core/file_assoc.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/lib/include/string.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"

// --- Inline helpers ---
static void exp_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}
static void exp_itoa(uint32_t val, char* buf) {
    char temp[16];
    int i = 0;
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0) buf[j++] = temp[--i];
    buf[j] = '\0';
}

// Color codes used to distinguish directories from files in buttons
#define EXP_DIR_BG_COLOR  0xFFE0F2FE
#define EXP_FILE_BG_COLOR 0xFFF1F5F9

typedef struct {
    uint32_t window_id;
    uint32_t toolbar_id;
    uint32_t pathbar_id;
    uint32_t list_panel_id;
    uint32_t status_id;
    
    char current_path[256];
} BOS_ExplorerContext;

static BOS_ExplorerContext* exp_ctx = 0;

static void explorer_load_directory(const char* path);

// --- Callbacks ---
static void btn_up_clicked(uint32_t btn_id) {
    if (!exp_ctx) return;
    if (strcmp(exp_ctx->current_path, "/") == 0) return; // already at root
    
    int last_slash = -1;
    for (int i = 0; exp_ctx->current_path[i] != '\0'; i++) {
        if (exp_ctx->current_path[i] == '/') last_slash = i;
    }
    
    if (last_slash > 0) {
        exp_ctx->current_path[last_slash] = '\0';
    } else if (last_slash == 0) {
        exp_ctx->current_path[1] = '\0'; // root
    }
    explorer_load_directory(exp_ctx->current_path);
}

static void btn_refresh_clicked(uint32_t btn_id) {
    if (!exp_ctx) return;
    explorer_load_directory(exp_ctx->current_path);
}

static void file_item_clicked(uint32_t btn_id) {
    if (!exp_ctx) return;
    BWE_Surface* btn = BWE_GetSurface(btn_id);
    if (!btn) return;
    
    const char* filename = btn->control_data.button.text;
    uint32_t bg = btn->control_data.button.bg_color;
    
    // Build full path
    char full_path[256];
    strcpy(full_path, exp_ctx->current_path);
    int len = strlen(full_path);
    if (full_path[len - 1] != '/') {
        full_path[len] = '/';
        full_path[len + 1] = '\0';
    }
    exp_strcat(full_path, filename);
    
    if (bg == EXP_DIR_BG_COLOR) {
        // It's a directory — navigate into it
        explorer_load_directory(full_path);
    } else {
        // It's a file — launch through File Association Engine
        BOS_OpenFile(full_path);
    }
}

static void explorer_load_directory(const char* path) {
    if (!exp_ctx) return;
    
    strcpy(exp_ctx->current_path, path);
    BOS_SetText(exp_ctx->pathbar_id, path);
    
    // Destroy old list panel and recreate it to clear items
    extern bwe_error_t BOS_DestroySurface(uint32_t surface_id);
    BOS_DestroySurface(exp_ctx->list_panel_id);
    
    BOS_CreatePanel(exp_ctx->window_id, 0, 70, 600, 310, 0xFFFFFFFF, &exp_ctx->list_panel_id);
    
    int index = 0;
    vfs_dirent_t entry;
    
    uint32_t x_offset = 10;
    uint32_t y_offset = 10;
    uint32_t item_count = 0;
    
    while (vfs_readdir(path, index, &entry) == 0) {
        if (strlen(entry.name) > 0) {
            uint32_t item_id;
            // Draw a button for the item
            // Light blue for folders, gray for files
            uint32_t bg_color = entry.is_directory ? 0xFFE0F2FE : 0xFFF1F5F9;
            BOS_CreateButton(exp_ctx->list_panel_id, x_offset, y_offset, 100, 40, entry.name, file_item_clicked, &item_id);
            BWE_Surface* btn = BWE_GetSurface(item_id);
            if (btn) {
                btn->control_data.button.bg_color = bg_color;
                btn->control_data.button.text_color = 0xFF0F172A; // Dark text
            }
            
            x_offset += 110;
            if (x_offset > 500) {
                x_offset = 10;
                y_offset += 50;
            }
            item_count++;
        }
        index++;
    }
    
    char status[64];
    strcpy(status, "Items: ");
    char count_str[16];
    exp_itoa(item_count, count_str);
    exp_strcat(status, count_str);
    BOS_SetText(exp_ctx->status_id, status);
}

// --- App Lifecycle ---
bwe_error_t explorer_init(uint32_t* out_win) {
    if (exp_ctx != 0) return 0x1008; // Already running
    
    exp_ctx = (BOS_ExplorerContext*)kmalloc(sizeof(BOS_ExplorerContext));
    if (!exp_ctx) return 0x1004;
    memset(exp_ctx, 0, sizeof(BOS_ExplorerContext));
    
    bwe_error_t err = BOS_CreateWindow(100, 100, 600, 400, "File Explorer", &exp_ctx->window_id);
    if (err != 0) { kfree(exp_ctx); exp_ctx = 0; return err; }
    
    // Toolbar Panel (Top)
    BOS_CreatePanel(exp_ctx->window_id, 0, 30, 600, 40, 0xFFF8FAFC, &exp_ctx->toolbar_id);
    
    // Navigation Buttons
    uint32_t btn_up, btn_ref;
    BOS_CreateButton(exp_ctx->toolbar_id, 10, 5, 40, 30, "Up", btn_up_clicked, &btn_up);
    BOS_CreateButton(exp_ctx->toolbar_id, 60, 5, 80, 30, "Refresh", btn_refresh_clicked, &btn_ref);
    
    // Path Bar
    BOS_CreateTextbox(exp_ctx->toolbar_id, 150, 5, 430, 30, "/", &exp_ctx->pathbar_id);
    
    // Status Bar Panel (Bottom)
    uint32_t status_panel;
    BOS_CreatePanel(exp_ctx->window_id, 0, 380, 600, 20, 0xFFE2E8F0, &status_panel);
    BOS_CreateLabel(status_panel, 10, 2, "Items: 0", 0xFF334155, &exp_ctx->status_id);
    
    // File List Panel (Middle)
    BOS_CreatePanel(exp_ctx->window_id, 0, 70, 600, 310, 0xFFFFFFFF, &exp_ctx->list_panel_id);
    
    // Load initial directory
    explorer_load_directory("/");
    
    if (out_win) *out_win = exp_ctx->window_id;
    return 0; // SUCCESS
}

void explorer_exit(void) {
    if (exp_ctx) {
        display_print("[APP] Explorer: Session closed\n");
        kfree(exp_ctx);
        exp_ctx = 0;
    }
}
