// ============================================================
// ATOMS OS Explorer — Pure Renderer (Phase 9 Rewrite)
// ============================================================
// This file contains ONLY drawing code.
// ZERO filesystem calls. ZERO VFS. ZERO NTFS. ZERO FAT32.
// All data comes from BSOMObject via ExplorerViewItem.
// ============================================================

#include "explorer_view.h"
#include "app_clipboard.h"
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
        if (path && (strcmp(path, "virtual://ThisPC") == 0 || strcmp(path, "This PC") == 0 || strcmp(path, "ThisPC") == 0)) {
            strcat(format_path, " > This PC");
        } else if (path && strcmp(path, "/") != 0) {
            strcat(format_path, " > ");
            if (path[0] == '/') {
                strcat(format_path, &path[1]);
            } else {
                strcat(format_path, path);
            }
        } else if (path && strcmp(path, "/") == 0) {
            strcat(format_path, " > A:\\ ATOMS Drive");
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

    const char* path = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "virtual://ThisPC";
    bool is_this_pc = (strcmp(path, "virtual://ThisPC") == 0 || strcmp(path, "This PC") == 0 || strcmp(path, "ThisPC") == 0);

    // ============================================================
    // 1. THIS PC — DEVICES AND DRIVES STORAGE HUB PANEL
    // ============================================================
    if (is_this_pc) {
        // Section Header
        BWE_DrawText(fb, "Devices and Drives", x + 16, y + 16, 0xFF1E293B, NULL);
        BWE_FillRect(fb, x + 16, y + 36, w - 32, 1, 0xFFE2E8F0);

        int32_t card_w = 175;
        int32_t card_h = 120;
        int32_t gap_x = 18;
        int32_t gap_y = 18;
        int32_t cols = (w - 32) / (card_w + gap_x);
        if (cols <= 0) cols = 1;

        int32_t start_y = y + 50 - ctx->scroll_y;

        for (uint32_t i = 0; i < ctx->view_item_count; i++) {
            ExplorerViewItem* vi = &ctx->view_items[i];
            if (!vi->obj) continue;

            int32_t row = i / cols;
            int32_t col = i % cols;

            int32_t card_x = x + 16 + (col * (card_w + gap_x));
            int32_t card_y = start_y + (row * (card_h + gap_y));

            if (card_y + card_h < y || card_y > y + h) continue;

            // Card Container (Hover / Selection state)
            uint32_t bg_col = vi->is_selected ? 0xFFE0F2FE : 0xFFF8FAFC;
            uint32_t border_col = vi->is_selected ? 0xFF0284C7 : 0xFFCBD5E1;
            uint32_t border_thick = vi->is_selected ? 2 : 1;

            BWE_FillRect(fb, card_x, card_y, card_w, card_h, bg_col);
            BWE_DrawRect(fb, card_x, card_y, card_w, card_h, border_col, border_thick);

            int32_t icon_x = card_x + (card_w - 44) / 2;
            int32_t icon_y = card_y + 14;

            // Render Large Device Icons based on Class / Type
            if (vi->obj->class_type == BSOM_CLASS_USB || vi->icon_id == 12) {
                // USB DRIVE ICON (USBdrive.png style: Metal Plug + Cyan Body + Green LED)
                BWE_FillRect(fb, icon_x + 14, icon_y, 16, 8, 0xFF94A3B8); // Plug
                BWE_DrawRect(fb, icon_x + 14, icon_y, 16, 8, 0xFF64748B, 1);
                BWE_FillRect(fb, icon_x + 18, icon_y + 2, 3, 4, 0xFF334155); // Pins
                BWE_FillRect(fb, icon_x + 23, icon_y + 2, 3, 4, 0xFF334155);

                BWE_FillRect(fb, icon_x + 4, icon_y + 8, 36, 30, 0xFF0284C7); // Flash Body
                BWE_DrawRect(fb, icon_x + 4, icon_y + 8, 36, 30, 0xFF0369A1, 1);

                BWE_FillRect(fb, icon_x + 8, icon_y + 12, 6, 6, 0xFF10B981); // Green LED
                BWE_DrawText(fb, "USB", icon_x + 12, icon_y + 22, 0xFFFFFFFF, NULL);
            } else if (vi->icon_id == 11 || strstr(vi->obj->name, "NTFS") != NULL) {
                // NTFS VOLUME ICON (Slate HDD + NTFS Badge)
                BWE_FillRect(fb, icon_x, icon_y + 4, 44, 34, 0xFF0F172A);
                BWE_DrawRect(fb, icon_x, icon_y + 4, 44, 34, 0xFF0284C7, 1);

                BWE_FillRect(fb, icon_x + 4, icon_y + 8, 36, 4, 0xFF38BDF8); // Activity Strip
                BWE_DrawText(fb, "NTFS", icon_x + 8, icon_y + 18, 0xFF38BDF8, NULL);
            } else {
                // SYSTEM DRIVE (A-DRIVE.png style: Silver Header + Dark Casing + Blue LED)
                BWE_FillRect(fb, icon_x, icon_y + 4, 44, 34, 0xFF1E293B); // Case
                BWE_DrawRect(fb, icon_x, icon_y + 4, 44, 34, 0xFF475569, 1);

                BWE_FillRect(fb, icon_x + 4, icon_y + 8, 36, 6, 0xFF94A3B8); // Platter
                BWE_FillRect(fb, icon_x + 8, icon_y + 20, 6, 6, 0xFF3B82F6); // Blue LED
                BWE_DrawText(fb, "SYSTEM", icon_x + 2, icon_y + 28, 0xFF60A5FA, NULL);
            }

            // Drive Name Typography
            char title_short[20];
            size_t n_len = strlen(vi->obj->name);
            if (n_len > 18) {
                strncpy(title_short, vi->obj->name, 15);
                title_short[15] = '.';
                title_short[16] = '.';
                title_short[17] = '\0';
            } else {
                strcpy(title_short, vi->obj->name);
            }

            int32_t tx = card_x + (card_w - (strlen(title_short) * 7)) / 2;
            if (tx < card_x + 4) tx = card_x + 4;

            BWE_DrawText(fb, title_short, tx, card_y + 56, 0xFF0F172A, NULL);

            // Drive Subtitle / Capacity
            const char* sub = (vi->icon_id == 12) ? "Removable USB Disk" : ((vi->icon_id == 11) ? "NTFS Storage Volume" : "System Partition (A:)");
            int32_t sx = card_x + (card_w - (strlen(sub) * 6)) / 2;
            if (sx < card_x + 2) sx = card_x + 2;

            BWE_DrawText(fb, sub, sx, card_y + 80, 0xFF64748B, NULL);
        }
        return;
    }

    // ============================================================
    // 2. STANDARD FILES AND FOLDERS GRID PANEL
    // ============================================================
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

        // Check if item is in Cut state
        bool is_cut = false;
        if (App_ClipboardIsCut()) {
            const char* clip_p = App_ClipboardGetPath();
            if (clip_p && clip_p[0] != '\0') {
                char item_p[256];
                if (strlen(vi->obj->path) > 0) strcpy(item_p, vi->obj->path);
                else {
                    const char* cur_p = (ctx->current_folder && strlen(ctx->current_folder->path) > 0) ? ctx->current_folder->path : "/";
                    if (strcmp(cur_p, "/") == 0) { strcpy(item_p, "/"); strcat(item_p, vi->obj->name); }
                    else { strcpy(item_p, cur_p); strcat(item_p, "/"); strcat(item_p, vi->obj->name); }
                }
                if (strcmp(item_p, clip_p) == 0) is_cut = true;
            }
        }

        // Selection backdrop (view state only)
        if (vi->is_selected) {
            BWE_FillRect(fb, file_x - 2, file_y - 2, item_w - 4, item_h - 4, is_cut ? 0x80316AC5 : 0xFF316AC5);
            BWE_DrawRect(fb, file_x - 2, file_y - 2, item_w - 4, item_h - 4, 0xFF0A246A, 1);
        }

        // Draw icon based on BSOM class type (Ghosted if cut)
        if (vi->obj->class_type == BSOM_CLASS_FOLDER ||
            vi->obj->class_type == BSOM_CLASS_DRIVE ||
            vi->obj->class_type == BSOM_CLASS_VIRTUAL) {
            // Folder icon (Ghosted palette if cut)
            uint32_t c_tab = is_cut ? 0xFFF59E0B : 0xFFD97706;
            uint32_t c_body = is_cut ? 0xFFFEF3C7 : 0xFFFCD34D;
            uint32_t c_border = is_cut ? 0xFFF59E0B : 0xFFB45309;
            BWE_FillRect(fb, file_x + 28, file_y + 4, 16, 6, c_tab);
            BWE_FillRect(fb, file_x + 24, file_y + 8, 36, 26, c_body);
            BWE_DrawRect(fb, file_x + 24, file_y + 8, 36, 26, c_border, 1);
        } else {
            // Document file icon (Ghosted palette if cut)
            uint32_t c_body = is_cut ? 0xFFF1F5F9 : 0xFFFFFFFF;
            uint32_t c_border = is_cut ? 0xFFCBD5E1 : 0xFF7F9DB9;
            uint32_t c_lines = is_cut ? 0x9093C5FD : 0xFF60A5FA;
            BWE_FillRect(fb, file_x + 28, file_y + 4, 28, 32, c_body);
            BWE_DrawRect(fb, file_x + 28, file_y + 4, 28, 32, c_border, 1);
            BWE_FillRect(fb, file_x + 32, file_y + 10, 20, 2, c_lines);
            BWE_FillRect(fb, file_x + 32, file_y + 16, 16, 2, c_lines);
            BWE_FillRect(fb, file_x + 32, file_y + 22, 18, 2, c_lines);
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

        uint32_t text_col = vi->is_selected ? 0xFFFFFFFF : (is_cut ? 0xFF94A3B8 : 0xFF000000);
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
// 6. Context Menu Renderer
// ------------------------------------------------------------
void Explorer_DrawContextMenu(BVFramebuffer* fb, ExplorerContext* ctx) {
    if (!ctx || !ctx->ctx_menu_open) return;

    int32_t cx = ctx->ctx_menu_x;
    int32_t cy = ctx->ctx_menu_y;
    int32_t cw = 150;
    int32_t ch = ctx->ctx_menu_is_item ? 162 : 112;

    // Outer dark acrylic card
    BWE_FillRect(fb, cx, cy, cw, ch, 0xF00F172A);
    BWE_DrawRect(fb, cx, cy, cw, ch, 0xFF475569, 1);

    if (ctx->ctx_menu_is_item) {
        const char* item_opts[] = { "  Open", "  Cut", "  Copy", "  Rename", "  Delete", "  Properties" };
        for (int i = 0; i < 6; i++) {
            int32_t iy = cy + 4 + (i * 26);
            BWE_DrawText(fb, item_opts[i], cx + 6, iy + 4, 0xFFF1F5F9, NULL);
        }
    } else {
        const char* bg_opts[] = { "  + New Folder", "  + New File", "  Paste", "  Refresh" };
        for (int i = 0; i < 4; i++) {
            int32_t iy = cy + 4 + (i * 26);
            BWE_DrawText(fb, bg_opts[i], cx + 6, iy + 4, 0xFFF1F5F9, NULL);
        }
    }
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
    Explorer_DrawContextMenu((BVFramebuffer*)fb, ctx);
}
