#include "text_viewer.h"
#include "kernel/BOSurface/Core/app_manager.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/lib/include/string.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"
#include "bovisual/Include/text.h"

#define TV_MAX_FILE_SIZE 4096
#define TV_MAX_LINES     128
#define TV_MAX_LINE_LEN  128
#define TV_MAX_INSTANCES 4

typedef struct {
    uint32_t window_id;
    uint32_t body_id;
    char     title[64];
    char     lines[TV_MAX_LINES][TV_MAX_LINE_LEN];
    uint32_t line_count;
    uint32_t scroll_offset;
    int      active;
} BOS_TextViewerCtx;

static BOS_TextViewerCtx tv_instances[TV_MAX_INSTANCES];
static int tv_initialized = 0;

static void tv_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void tv_strcat(char* dst, const char* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

static BOS_TextViewerCtx* tv_find_free(void) {
    for (int i = 0; i < TV_MAX_INSTANCES; i++) {
        if (!tv_instances[i].active) return &tv_instances[i];
    }
    return 0;
}

static BOS_TextViewerCtx* tv_find_by_window(uint32_t win_id) {
    for (int i = 0; i < TV_MAX_INSTANCES; i++) {
        if (tv_instances[i].active && tv_instances[i].window_id == win_id)
            return &tv_instances[i];
    }
    // Also check body_id
    for (int i = 0; i < TV_MAX_INSTANCES; i++) {
        if (tv_instances[i].active && tv_instances[i].body_id == win_id)
            return &tv_instances[i];
    }
    return 0;
}

// Render hook for the text body panel
static void tv_render_hook(BWE_Surface* surface) {
    if (!surface) return;
    
    // Find which instance this body panel belongs to
    BOS_TextViewerCtx* ctx = 0;
    for (int i = 0; i < TV_MAX_INSTANCES; i++) {
        if (tv_instances[i].active && tv_instances[i].body_id == surface->id) {
            ctx = &tv_instances[i];
            break;
        }
    }
    if (!ctx) return;
    
    uint32_t text_color = 0xFF1E293B; // Slate-800
    const BVFontMetrics* font = BV_GetDefaultFont();
    uint32_t line_height = font ? font->line_height : 16;
    uint32_t pad_x = 8;
    uint32_t pad_y = 6;
    uint32_t visible_lines = (surface->screen_bounds.height > 2 * pad_y)
                             ? (surface->screen_bounds.height - 2 * pad_y) / line_height
                             : 1;
    
    uint32_t start = ctx->scroll_offset;
    
    for (uint32_t i = 0; i < visible_lines && (start + i) < ctx->line_count; i++) {
        BOVISUAL_Draw_String(
            surface->screen_bounds.x + (int32_t)pad_x,
            surface->screen_bounds.y + (int32_t)pad_y + (int32_t)(i * line_height),
            ctx->lines[start + i],
            text_color, 0, true, font);
    }
    
    // If no content
    if (ctx->line_count == 0) {
        BOVISUAL_Draw_String(
            surface->screen_bounds.x + (int32_t)pad_x,
            surface->screen_bounds.y + (int32_t)pad_y,
            "(empty file)",
            0xFF94A3B8, 0, true, font);
    }
}

// Event hook for scrolling with keyboard
static void tv_event_hook(uint32_t surface_id, const BVEvent* event) {
    if (!event) return;
    if (event->type != BV_EVENT_KEY_DOWN) return;
    
    BOS_TextViewerCtx* ctx = tv_find_by_window(surface_id);
    if (!ctx) return;
    
    const BVFontMetrics* font = BV_GetDefaultFont();
    uint32_t line_height = font ? font->line_height : 16;
    BWE_Surface* body = BWE_GetSurface(ctx->body_id);
    uint32_t visible = body ? (body->screen_bounds.height - 12) / line_height : 10;
    
    uint8_t key = event->key_code;
    bool changed = false;
    
    // Arrow up (0x80), Page Up (0x93), W, K
    if (key == 0x80 || key == 'w' || key == 'W' || key == 'k' || key == 'K') {
        if (ctx->scroll_offset > 0) {
            ctx->scroll_offset--;
            changed = true;
        }
    }
    else if (key == 0x93) { // Page Up
        if (ctx->scroll_offset > visible) ctx->scroll_offset -= visible;
        else ctx->scroll_offset = 0;
        changed = true;
    }
    // Arrow down (0x81), Page Down (0x94), S, J
    else if (key == 0x81 || key == 's' || key == 'S' || key == 'j' || key == 'J') {
        if (ctx->scroll_offset + visible < ctx->line_count) {
            ctx->scroll_offset++;
            changed = true;
        }
    }
    else if (key == 0x94) { // Page Down
        if (ctx->scroll_offset + 2 * visible < ctx->line_count) ctx->scroll_offset += visible;
        else if (ctx->line_count > visible) ctx->scroll_offset = ctx->line_count - visible;
        changed = true;
    }
    
    if (changed) {
        // State change will automatically be picked up by the next BOHEART pulse
    }
}

// Parse raw file buffer into lines with automatic wrapping
static void tv_parse_lines(BOS_TextViewerCtx* ctx, const char* data, uint32_t size) {
    ctx->line_count = 0;
    uint32_t col = 0;
    uint32_t max_cols = 56; // Safe limit for 500px window width
    
    for (uint32_t i = 0; i < size && ctx->line_count < TV_MAX_LINES; i++) {
        if (data[i] == '\n') {
            ctx->lines[ctx->line_count][col] = '\0';
            ctx->line_count++;
            col = 0;
        } else if (data[i] == '\r') {
            // skip CR
        } else {
            ctx->lines[ctx->line_count][col++] = data[i];
            // Wrap on space after col 45, or force wrap at max_cols
            if (col >= max_cols || (col >= 45 && data[i] == ' ')) {
                ctx->lines[ctx->line_count][col] = '\0';
                ctx->line_count++;
                col = 0;
            }
        }
    }
    // Flush last line
    if (col > 0 && ctx->line_count < TV_MAX_LINES) {
        ctx->lines[ctx->line_count][col] = '\0';
        ctx->line_count++;
    }
}

void text_viewer_open(const char* filepath) {
    if (!filepath) return;
    
    if (!tv_initialized) {
        for (int i = 0; i < TV_MAX_INSTANCES; i++) {
            tv_instances[i].active = 0;
        }
        tv_initialized = 1;
    }
    
    BOS_TextViewerCtx* ctx = tv_find_free();
    if (!ctx) {
        display_print("[TVIEW] Max instances reached\n");
        return;
    }
    
    memset(ctx, 0, sizeof(BOS_TextViewerCtx));
    ctx->active = 1;
    
    // Extract filename from path for title
    const char* fname = filepath;
    const char* p = filepath;
    while (*p) { if (*p == '/') fname = p + 1; p++; }
    
    strcpy(ctx->title, "Text Viewer - ");
    tv_strcat(ctx->title, fname);
    
    // Read file from VFS
    char* file_buf = (char*)kmalloc(TV_MAX_FILE_SIZE);
    if (!file_buf) {
        display_print("[TVIEW] Out of memory\n");
        ctx->active = 0;
        return;
    }
    memset(file_buf, 0, TV_MAX_FILE_SIZE);
    
    int fd = vfs_open(filepath);
    uint32_t bytes_read = 0;
    if (fd >= 0) {
        int r = vfs_read(fd, file_buf, TV_MAX_FILE_SIZE - 1);
        if (r > 0) bytes_read = (uint32_t)r;
        vfs_close(fd);
    }
    file_buf[bytes_read] = '\0';
    
    // Parse into lines
    tv_parse_lines(ctx, file_buf, bytes_read);
    kfree(file_buf);
    
    display_print("[TVIEW] Loaded ");
    display_print(fname);
    display_print(" (");
    display_print_dec(bytes_read);
    display_print(" bytes, ");
    display_print_dec(ctx->line_count);
    display_print(" lines)\n");
    
    // Create window
    // Offset each new instance slightly for cascade effect
    static int instance_offset = 0;
    int32_t wx = 180 + (instance_offset * 30);
    int32_t wy = 80 + (instance_offset * 25);
    instance_offset = (instance_offset + 1) % 5;
    
    bwe_error_t err = BOS_CreateWindow(wx, wy, 500, 350, ctx->title, &ctx->window_id);
    if (err != BWE_SUCCESS) {
        display_print("[TVIEW] Failed to create window\n");
        ctx->active = 0;
        return;
    }
    
    // Body panel (white background for text)
    err = BOS_CreatePanel(ctx->window_id, 0, 30, 500, 320, 0xFFFFFBEB, &ctx->body_id);
    if (err != BWE_SUCCESS) {
        ctx->active = 0;
        return;
    }
    
    // Attach hooks
    BWE_Surface* win = BWE_GetSurface(ctx->window_id);
    if (win) {
        win->on_event = tv_event_hook;
    }
    BWE_Surface* body = BWE_GetSurface(ctx->body_id);
    if (body) {
        body->on_render = tv_render_hook;
        body->on_event = tv_event_hook;
    }
    
    // Focus the new window (composition happens automatically on BOHEART pulse)
    BOS_SetFocus(ctx->window_id);
}
