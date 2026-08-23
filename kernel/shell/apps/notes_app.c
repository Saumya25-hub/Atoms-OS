// ============================================================
// ATOMS OS — Notes.BOSX Notepad & Text Editor Application
// ============================================================
// Full VFS read/write text editor with toolbar and keyboard input.
// ============================================================

#include "notes_app.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/wm/bwe/include/bwe.h"

#define NOTES_MAX_BUF_SIZE 16384
#define NOTES_MAX_INSTANCES 4

typedef struct {
    uint32_t window_id;
    uint32_t canvas_id;
    char     filepath[256];
    char     filename[64];
    char     buffer[NOTES_MAX_BUF_SIZE];
    uint32_t buf_len;
    uint32_t cursor_pos;
    bool     is_modified;
    bool     active;
} NotesContext;

static NotesContext s_notes_instances[NOTES_MAX_INSTANCES];
static bool s_notes_initialized = false;

extern const BVFramebuffer* BWE_GetRenderTarget(void);
extern void display_print(const char* str);
extern void Shell_ShowNotification(const char* title, const char* message, uint32_t duration_ms);

static NotesContext* notes_find_free(void) {
    for (int i = 0; i < NOTES_MAX_INSTANCES; i++) {
        if (!s_notes_instances[i].active) return &s_notes_instances[i];
    }
    return NULL;
}

static NotesContext* notes_find_by_win_id(uint32_t win_id) {
    for (int i = 0; i < NOTES_MAX_INSTANCES; i++) {
        if (s_notes_instances[i].active && s_notes_instances[i].window_id == win_id) {
            return &s_notes_instances[i];
        }
    }
    return NULL;
}

// ------------------------------------------------------------
// Save File to VFS
// ------------------------------------------------------------
static bool notes_save_file(NotesContext* ctx) {
    if (!ctx || ctx->filepath[0] == '\0') return false;

    // Create file if it doesn't exist
    vfs_create(ctx->filepath);

    int fd = vfs_open(ctx->filepath);
    if (fd < 0) {
        Shell_ShowNotification("Notes Error", "Failed to open file for writing", 3000);
        return false;
    }

    vfs_write(fd, ctx->buffer, ctx->buf_len);
    vfs_close(fd);

    ctx->is_modified = false;
    Shell_ShowNotification("Notes.BOSX", "File saved successfully!", 3000);
    return true;
}

// ------------------------------------------------------------
// Load File from VFS
// ------------------------------------------------------------
static bool notes_load_file(NotesContext* ctx, const char* path) {
    if (!ctx || !path) return false;

    strcpy(ctx->filepath, path);
    const char* p = path;
    const char* fname = path;
    while (*p) { if (*p == '/' || *p == '\\') fname = p + 1; p++; }
    strcpy(ctx->filename, fname);

    int fd = vfs_open(path);
    if (fd >= 0) {
        int bytes = vfs_read(fd, ctx->buffer, NOTES_MAX_BUF_SIZE - 1);
        vfs_close(fd);
        if (bytes > 0) {
            ctx->buf_len = bytes;
            ctx->buffer[bytes] = '\0';
        } else {
            ctx->buf_len = 0;
            ctx->buffer[0] = '\0';
        }
    } else {
        ctx->buf_len = 0;
        ctx->buffer[0] = '\0';
    }

    ctx->cursor_pos = ctx->buf_len;
    ctx->is_modified = false;
    return true;
}

// ------------------------------------------------------------
// Paint / Render Handler
// ------------------------------------------------------------
static void notes_render_cb(BWE_Window* win) {
    if (!win || !win->user_data) return;
    const BVFramebuffer* fb = BWE_GetRenderTarget();
    if (!fb) return;

    NotesContext* ctx = (NotesContext*)win->user_data;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;
    int32_t bw = win->screen_bounds.width - 10;
    int32_t bh = win->screen_bounds.height - 40;

    if (bw <= 0 || bh <= 0) return;

    // 1. Toolbar Background
    BWE_FillRect(fb, bx, by, bw, 32, 0xFF1E293B);
    BWE_DrawRect(fb, bx, by + 31, bw, 1, 0xFF475569, 1);

    // Save Button
    BWE_FillRect(fb, bx + 6, by + 4, 60, 24, 0xFF3B82F6);
    BWE_DrawRect(fb, bx + 6, by + 4, 60, 24, 0xFF1D4ED8, 1);
    BWE_DrawText(fb, "Save", bx + 20, by + 8, 0xFFFFFFFF, NULL);

    // Reload Button
    BWE_FillRect(fb, bx + 72, by + 4, 65, 24, 0xFF334155);
    BWE_DrawRect(fb, bx + 72, by + 4, 65, 24, 0xFF475569, 1);
    BWE_DrawText(fb, "Reload", bx + 84, by + 8, 0xFFF1F5F9, NULL);

    // File info label
    char title_str[128];
    strcpy(title_str, "File: ");
    strcat(title_str, ctx->filename[0] != '\0' ? ctx->filename : "Untitled");
    if (ctx->is_modified) strcat(title_str, " *");
    BWE_DrawText(fb, title_str, bx + 150, by + 8, 0xFF94A3B8, NULL);

    // 2. Text Editor Body Area
    int32_t body_y = by + 32;
    int32_t body_h = bh - 32;
    BWE_FillRect(fb, bx, body_y, bw, body_h, 0xFF0F172A);

    // Render lines of text
    int32_t tx = bx + 10;
    int32_t ty = body_y + 10;
    char line_buf[128];
    uint32_t line_pos = 0;

    for (uint32_t i = 0; i <= ctx->buf_len && ty < body_y + body_h - 20; i++) {
        char ch = ctx->buffer[i];
        if (ch == '\n' || ch == '\0' || line_pos >= 100) {
            line_buf[line_pos] = '\0';
            BWE_DrawText(fb, line_buf, tx, ty, 0xFFF8FAFC, NULL);
            ty += 20;
            line_pos = 0;
        } else if (ch != '\r') {
            line_buf[line_pos++] = ch;
        }
    }
}

// ------------------------------------------------------------
// Event Handler
// ------------------------------------------------------------
static void notes_event_cb(uint32_t win_id, const BWE_Event* event) {
    if (!event) return;
    NotesContext* ctx = notes_find_by_win_id(win_id);
    if (!ctx) return;

    BWE_Window* win = BWE_GetWindow(win_id);
    if (!win) return;

    int32_t bx = win->screen_bounds.x + 5;
    int32_t by = win->screen_bounds.y + 35;

    if (event->type == BWE_EVENT_MOUSE_DOWN) {
        int32_t mx = event->data.mouse.x;
        int32_t my = event->data.mouse.y;

        // Toolbar Click
        if (my >= by + 4 && my <= by + 28) {
            int32_t rx = mx - bx;
            if (rx >= 6 && rx <= 66) { // Save
                notes_save_file(ctx);
                BWE_InvalidateWindow(win_id);
            } else if (rx >= 72 && rx <= 137) { // Reload
                if (ctx->filepath[0] != '\0') {
                    notes_load_file(ctx, ctx->filepath);
                    BWE_InvalidateWindow(win_id);
                }
            }
        }
    } else if (event->type == BWE_EVENT_KEY_DOWN) {
        uint32_t kc = event->data.key.key_code;
        uint32_t ch = event->data.key.character;

        if (kc == 0x08 || kc == 8 || kc == 0x0E || kc == 127 || ch == 8 || ch == '\b') { // Backspace
            if (ctx->buf_len > 0) {
                ctx->buf_len--;
                ctx->buffer[ctx->buf_len] = '\0';
                ctx->is_modified = true;
                BWE_InvalidateWindow(win_id);
            }
        } else if (kc == 13 || kc == 10) { // Enter / Newline
            if (ctx->buf_len < NOTES_MAX_BUF_SIZE - 2) {
                ctx->buffer[ctx->buf_len++] = '\n';
                ctx->buffer[ctx->buf_len] = '\0';
                ctx->is_modified = true;
                BWE_InvalidateWindow(win_id);
            }
        } else if (ch >= 32 && ch <= 126) { // Printable characters
            if (ctx->buf_len < NOTES_MAX_BUF_SIZE - 2) {
                ctx->buffer[ctx->buf_len++] = (char)ch;
                ctx->buffer[ctx->buf_len] = '\0';
                ctx->is_modified = true;
                BWE_InvalidateWindow(win_id);
            }
        }
    }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
uint32_t notes_app_open(const char* filepath) {
    if (!s_notes_initialized) {
        memset(s_notes_instances, 0, sizeof(s_notes_instances));
        s_notes_initialized = true;
    }

    NotesContext* ctx = notes_find_free();
    if (!ctx) {
        Shell_ShowNotification("Notes.BOSX", "Maximum editor windows open", 3000);
        return 0;
    }

    memset(ctx, 0, sizeof(NotesContext));
    ctx->active = true;

    bwe_error_t err = BOS_CreateWindow(120, 80, 640, 480, "Notes.BOSX - Text Editor", &ctx->window_id);
    if (err != 0) {
        ctx->active = false;
        return 0;
    }

    BWE_Window* win = BWE_GetWindow(ctx->window_id);
    if (win) {
        win->user_data = ctx;
        win->on_render = notes_render_cb;
        win->on_event  = notes_event_cb;
    }

    if (filepath && filepath[0] != '\0') {
        notes_load_file(ctx, filepath);
    } else {
        strcpy(ctx->filename, "New Document.txt");
        strcpy(ctx->filepath, "/desktop/New Document.txt");
        ctx->buffer[0] = '\0';
        ctx->buf_len = 0;
    }

    if (win) {
        char title_buf[128];
        strcpy(title_buf, "Notes.BOSX - ");
        strcat(title_buf, ctx->filename);
        strcpy(win->title, title_buf);
    }

    BOS_Show(ctx->window_id);
    BOS_SetFocus(ctx->window_id);
    return ctx->window_id;
}

int notes_app_launch(uint32_t* out_win) {
    uint32_t wid = notes_app_open("/desktop/New Document.txt");
    if (out_win) *out_win = wid;
    return 0;
}
