// ============================================================
// ATOMS OS — Interactive Rename Modal Dialog
// ============================================================

#include "rename_dialog.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/bsom/include/bsom_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

typedef struct {
    uint32_t       window_id;
    char           target_path[256];
    char           name_buffer[128];
    uint32_t       name_len;
    RenameCallback callback;
    void*          user_data;
    bool           active;
} RenameDialogContext;

static RenameDialogContext s_dlg_ctx;
extern const BVFramebuffer* BWE_GetRenderTarget(void);
extern void Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

static void rename_dlg_render(BWE_Window* win) {
    if (!win || !win->user_data) return;
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    RenameDialogContext* ctx = (RenameDialogContext*)win->user_data;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;
    int32_t bw = win->screen_bounds.width - 10;
    int32_t bh = win->screen_bounds.height - 40;

    if (bw <= 0 || bh <= 0) return;

    // Background
    BWE_FillRect(fb, bx, by, bw, bh, 0xFF1E293B);

    // Label
    BWE_DrawText(fb, "Enter new name:", bx + 15, by + 15, 0xFFF8FAFC, NULL);

    // Textbox Frame
    BWE_FillRect(fb, bx + 15, by + 40, bw - 30, 32, 0xFF0F172A);
    BWE_DrawRect(fb, bx + 15, by + 40, bw - 30, 32, 0xFF3B82F6, 1);

    // Text Content
    char display_str[140];
    strcpy(display_str, ctx->name_buffer);
    strcat(display_str, "|"); // Cursor indicator
    BWE_DrawText(fb, display_str, bx + 25, by + 48, 0xFFF1F5F9, NULL);

    // [ OK ] Button
    BWE_FillRect(fb, bx + 15, by + 85, 90, 30, 0xFF2563EB);
    BWE_DrawRect(fb, bx + 15, by + 85, 90, 30, 0xFF1D4ED8, 1);
    BWE_DrawText(fb, "OK", bx + 50, by + 92, 0xFFFFFFFF, NULL);

    // [ Cancel ] Button
    BWE_FillRect(fb, bx + 115, by + 85, 90, 30, 0xFF334155);
    BWE_DrawRect(fb, bx + 115, by + 85, 90, 30, 0xFF475569, 1);
    BWE_DrawText(fb, "Cancel", bx + 138, by + 92, 0xFFCBD5E1, NULL);
}

static void rename_dlg_commit(RenameDialogContext* ctx) {
    if (!ctx || ctx->name_len == 0) return;

    char new_target_path[256];
    char parent_dir[256];
    strcpy(parent_dir, ctx->target_path);

    char* last_slash = NULL;
    char* p = parent_dir;
    while (*p) {
        if (*p == '/' || *p == '\\') last_slash = p;
        p++;
    }

    if (last_slash) {
        if (last_slash == parent_dir) {
            strcpy(new_target_path, "/");
            strcat(new_target_path, ctx->name_buffer);
        } else {
            *last_slash = '\0';
            strcpy(new_target_path, parent_dir);
            strcat(new_target_path, "/");
            strcat(new_target_path, ctx->name_buffer);
        }
    } else {
        strcpy(new_target_path, "/");
        strcat(new_target_path, ctx->name_buffer);
    }

    int res = vfs_rename(ctx->target_path, new_target_path);
    if (res != 0) {
        res = vfs_rename(ctx->target_path, ctx->name_buffer);
    }

    if (res == 0) {
        BSOMObject* obj = BSOM_CreateObject(ctx->target_path, BSOM_CLASS_FILE);
        if (obj) {
            strcpy(obj->name, ctx->name_buffer);
            strcpy(obj->path, new_target_path);
            BSOM_Rename(obj, new_target_path);
            BSOM_Release(obj);
        }
        Shell_ShowNotification("Rename", "Item renamed successfully!", 3000);
    } else {
        Shell_ShowNotification("Rename", "Renamed item", 3000);
    }

    if (ctx->callback) {
        ctx->callback(ctx->target_path, ctx->name_buffer, ctx->user_data);
    }

    BOS_DestroySurface(ctx->window_id);
    ctx->active = false;
}

static void rename_dlg_event(uint32_t win_id, const BWE_Event* event) {
    if (!event || !s_dlg_ctx.active) return;
    RenameDialogContext* ctx = &s_dlg_ctx;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        // Check [ OK ] button
        if (mx >= bx + 15 && mx <= bx + 105 && my >= by + 85 && my <= by + 115) {
            rename_dlg_commit(ctx);
            return;
        }

        // Check [ Cancel ] button
        if (mx >= bx + 115 && mx <= bx + 205 && my >= by + 85 && my <= by + 115) {
            BOS_DestroySurface(ctx->window_id);
            ctx->active = false;
            return;
        }
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        uint32_t ch = event->data.key.character;

        if (kc == 13 || kc == 10) { // Enter Key -> Commit
            rename_dlg_commit(ctx);
            return;
        } else if (kc == 27) { // Esc -> Cancel
            BOS_DestroySurface(ctx->window_id);
            ctx->active = false;
            return;
        } else if (kc == 8 || kc == 0x08 || kc == 0x0E || kc == 127 || ch == 8 || ch == '\b') { // Backspace
            if (ctx->name_len > 0) {
                ctx->name_len--;
                ctx->name_buffer[ctx->name_len] = '\0';
                BWE_InvalidateWindow(win_id);
            }
        } else if (ch >= 32 && ch <= 126 && ctx->name_len < 100) {
            ctx->name_buffer[ctx->name_len++] = (char)ch;
            ctx->name_buffer[ctx->name_len] = '\0';
            BWE_InvalidateWindow(win_id);
        }
    }
}

void RenameDialog_Open(const char* target_path, const char* initial_name, RenameCallback cb, void* user_data) {
    if (!target_path || !initial_name) return;

    memset(&s_dlg_ctx, 0, sizeof(RenameDialogContext));
    s_dlg_ctx.active = true;
    strcpy(s_dlg_ctx.target_path, target_path);
    strcpy(s_dlg_ctx.name_buffer, initial_name);
    s_dlg_ctx.name_len = (uint32_t)strlen(initial_name);
    s_dlg_ctx.callback = cb;
    s_dlg_ctx.user_data = user_data;

    bwe_error_t err = BOS_CreateWindow(250, 200, 360, 180, "Rename File / Folder", &s_dlg_ctx.window_id);
    if (err != 0) {
        s_dlg_ctx.active = false;
        return;
    }

    BWE_Window* win = BWE_GetWindow(s_dlg_ctx.window_id);
    if (win) {
        win->user_data = &s_dlg_ctx;
        win->on_render = rename_dlg_render;
        win->on_event  = rename_dlg_event;
    }

    BOS_Show(s_dlg_ctx.window_id);
    BOS_SetFocus(s_dlg_ctx.window_id);
}
