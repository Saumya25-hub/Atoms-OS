// ============================================================
// ATOMS OS Explorer — Pure Renderer (Phase 9 Rewrite)
// ============================================================
// This file contains ONLY drawing code.
// ZERO filesystem calls. ZERO VFS. ZERO NTFS. ZERO FAT32.
// All data comes from BSOMObject via ExplorerViewItem.
// ============================================================

#include "explorer_view.h"
#include "kernel/core/lib/include/string.h"

extern const BVFramebuffer* BWE_GetRenderTarget(void);

// ------------------------------------------------------------
// 1. Toolbar Renderer (< > Up Refresh)
// ------------------------------------------------------------
void Explorer_DrawToolbar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx) {
    (void)ctx;
    BWE_FillRect(fb, x, y, w, h, 0xFFECE9D8);
    BWE_DrawRect(fb, x, y + h - 1, w, 1, 0xFFACA899, 1);

    // [<] Back
    BWE_FillRect(fb, x + 8, y + 4, 32, 26, 0xFFF5F5F5);
    BWE_DrawRect(fb, x + 8, y + 4, 32, 26, 0xFF7F9DB9, 1);
    BWE_DrawText(fb, "<", x + 18, y + 10, 0xFF000000, NULL);

    // [>] Forward
    BWE_FillRect(fb, x + 44, y + 4, 32, 26, 0xFFF5F5F5);
    BWE_DrawRect(fb, x + 44, y + 4, 32, 26, 0xFF7F9DB9, 1);
    BWE_DrawText(fb, ">", x + 54, y + 10, 0xFF000000, NULL);

    // [Up] Parent
    BWE_FillRect(fb, x + 80, y + 4, 48, 26, 0xFFF5F5F5);
    BWE_DrawRect(fb, x + 80, y + 4, 48, 26, 0xFF7F9DB9, 1);
    BWE_DrawText(fb, "Up", x + 94, y + 10, 0xFF000000, NULL);

    // [Refresh]
    BWE_FillRect(fb, x + 132, y + 4, 64, 26, 0xFFF5F5F5);
    BWE_DrawRect(fb, x + 132, y + 4, 64, 26, 0xFF7F9DB9, 1);
    BWE_DrawText(fb, "Refresh", x + 140, y + 10, 0xFF000000, NULL);
}

// ------------------------------------------------------------
// 2. Address Bar Renderer
// ------------------------------------------------------------
void Explorer_DrawAddressBar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx) {
    BWE_FillRect(fb, x, y, w, h, 0xFFECE9D8);
    BWE_DrawRect(fb, x, y + h - 1, w, 1, 0xFFACA899, 1);

    BWE_DrawText(fb, "Address", x + 8, y + 7, 0xFF444444, NULL);

    int32_t box_x = x + 65;
    int32_t box_w = w - 75;
    BWE_FillRect(fb, box_x, y + 3, box_w, 22, 0xFFFFFFFF);
    BWE_DrawRect(fb, box_x, y + 3, box_w, 22, 0xFF7F9DB9, 1);

    // Display path from current BSOM folder object
    char format_path[256];
    strcpy(format_path, "Home");
    if (ctx && ctx->current_folder) {
        const char* path = ctx->current_folder->path;
        if (path && strcmp(path, "/") != 0) {
            strcat(format_path, " > ");
            if (path[0] == '/') {
                strcat(format_path, &path[1]);
            } else {
                strcat(format_path, path);
            }
        }
    }
    BWE_DrawText(fb, format_path, box_x + 6, y + 7, 0xFF000000, NULL);
}

// ------------------------------------------------------------
// 3. Sidebar Renderer
// ------------------------------------------------------------
void Explorer_DrawSidebar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx) {
    (void)ctx;
    BWE_FillRect(fb, x, y, w, h, 0xFF6B89D6);
    BWE_DrawRect(fb, x + w - 1, y, 1, h, 0xFF4261B5, 1);

    BWE_FillRect(fb, x + 8, y + 8, w - 16, 24, 0xFF215DC6);
    BWE_DrawText(fb, "Places", x + 14, y + 14, 0xFFFFFFFF, NULL);

    const char* sidebar_items[] = {
        "This PC", "Desktop", "Documents", "Downloads",
        "Music", "Pictures", "Videos", "Recycle Bin"
    };

    int32_t item_y = y + 36;
    for (int i = 0; i < 8; i++) {
        BWE_FillRect(fb, x + 8, item_y, w - 16, 26, 0xFFD6DDF8);
        BWE_DrawRect(fb, x + 8, item_y, w - 16, 26, 0xFF96ABEA, 1);
        BWE_FillRect(fb, x + 14, item_y + 9, 8, 8, 0xFF215DC6);
        BWE_DrawText(fb, sidebar_items[i], x + 28, item_y + 6, 0xFF0B256B, NULL);
        item_y += 30;
    }
}

// ------------------------------------------------------------
// 4. Main Files Grid Renderer — Renders BSOMObject View Items
// ------------------------------------------------------------
void Explorer_DrawFiles(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx) {
    if (!ctx) return;

    BWE_FillRect(fb, x, y, w, h, 0xFFFFFFFF);

    int32_t item_w = 90;
    int32_t item_h = 80;
    int32_t cols = w / item_w;
    if (cols <= 0) cols = 1;

    int32_t y_offset = -ctx->scroll_y;

    for (uint32_t i = 0; i < ctx->view_item_count; i++) {
        ExplorerViewItem* vi = &ctx->view_items[i];
        if (!vi->obj) continue;

        int32_t row = i / cols;
        int32_t col = i % cols;

        int32_t rel_y = (row * item_h) + 10 + y_offset;
        int32_t file_x = x + (col * item_w) + 8;
        int32_t file_y = y + rel_y;

        // Viewport clipping
        if (rel_y + item_h < 0 || rel_y > h) continue;

        // Selection backdrop (view state only)
        if (vi->is_selected) {
            BWE_FillRect(fb, file_x - 2, file_y - 2, item_w - 4, item_h - 4, 0xFF316AC5);
            BWE_DrawRect(fb, file_x - 2, file_y - 2, item_w - 4, item_h - 4, 0xFF0A246A, 1);
        }

        // Draw icon based on BSOM class type (NO filesystem logic)
        if (vi->obj->class_type == BSOM_CLASS_FOLDER ||
            vi->obj->class_type == BSOM_CLASS_DRIVE ||
            vi->obj->class_type == BSOM_CLASS_VIRTUAL) {
            // Folder icon
            BWE_FillRect(fb, file_x + 28, file_y + 4, 16, 6, 0xFFD97706);
            BWE_FillRect(fb, file_x + 24, file_y + 8, 36, 26, 0xFFFCD34D);
            BWE_DrawRect(fb, file_x + 24, file_y + 8, 36, 26, 0xFFB45309, 1);
        } else {
            // Document file icon
            BWE_FillRect(fb, file_x + 28, file_y + 4, 28, 32, 0xFFFFFFFF);
            BWE_DrawRect(fb, file_x + 28, file_y + 4, 28, 32, 0xFF7F9DB9, 1);
            BWE_FillRect(fb, file_x + 32, file_y + 10, 20, 2, 0xFF60A5FA);
            BWE_FillRect(fb, file_x + 32, file_y + 16, 16, 2, 0xFF60A5FA);
            BWE_FillRect(fb, file_x + 32, file_y + 22, 18, 2, 0xFF60A5FA);
        }

        // Truncate filename
        char short_name[14];
        size_t name_len = strlen(vi->obj->name);
        if (name_len > 10) {
            strncpy(short_name, vi->obj->name, 8);
            short_name[8] = '.';
            short_name[9] = '.';
            short_name[10] = '\0';
        } else {
            strcpy(short_name, vi->obj->name);
        }

        uint32_t text_col = vi->is_selected ? 0xFFFFFFFF : 0xFF000000;
        int32_t text_x = file_x + (item_w - (strlen(short_name) * 7)) / 2;
        if (text_x < file_x) text_x = file_x;

        BWE_DrawText(fb, short_name, text_x, file_y + 42, text_col, NULL);
    }
}

// ------------------------------------------------------------
// Helper
// ------------------------------------------------------------
static void itoa_simple(int val, char* str) {
    if (val == 0) { str[0] = '0'; str[1] = '\0'; return; }
    char temp[16]; int i = 0;
    while (val > 0) { temp[i++] = (val % 10) + '0'; val /= 10; }
    int j = 0;
    while (i > 0) { str[j++] = temp[--i]; }
    str[j] = '\0';
}

// ------------------------------------------------------------
// 5. Status Bar Renderer
// ------------------------------------------------------------
void Explorer_DrawStatusbar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx) {
    BWE_FillRect(fb, x, y, w, h, 0xFFECE9D8);
    BWE_DrawRect(fb, x, y, w, 1, 0xFFACA899, 1);

    uint32_t count = ctx ? ctx->view_item_count : 0;
    char status_str[64];
    char num_str[16];
    itoa_simple((int)count, num_str);
    strcpy(status_str, num_str);
    strcat(status_str, " Objects (BSOM)");

    BWE_DrawText(fb, status_str, x + 12, y + 6, 0xFF444444, NULL);
}

// ------------------------------------------------------------
// Master Render Callback
// ------------------------------------------------------------
void Explorer_RenderWindow(BWE_Window* win) {
    if (!win || !win->user_data) return;

    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    ExplorerContext* ctx = (ExplorerContext*)win->user_data;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;
    int32_t bw = win->screen_bounds.width - 10;
    int32_t bh = win->screen_bounds.height - 40;

    if (bw <= 0 || bh <= 0) return;

    Explorer_DrawToolbar((BVFramebuffer*)fb, bx, by, bw, 34, ctx);
    Explorer_DrawAddressBar((BVFramebuffer*)fb, bx, by + 34, bw, 28, ctx);
    Explorer_DrawSidebar((BVFramebuffer*)fb, bx, by + 62, 170, bh - 88, ctx);
    Explorer_DrawFiles((BVFramebuffer*)fb, bx + 170, by + 62, bw - 170, bh - 88, ctx);
    Explorer_DrawStatusbar((BVFramebuffer*)fb, bx, by + bh - 26, bw, 26, ctx);
}
