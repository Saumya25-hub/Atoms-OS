#include "terminal.h"
#include "kernel/shell/conhost/conhost.h"
#include "kernel/wm/surface/app_manager.h"
#include "bovisual/Include/text.h"
#include "bovisual/Include/graphics.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/drivers/display/display.h"

extern uint32_t g_current_creating_pid;

// ============================================================
// Render Hook — Draw terminal buffer on body panel
// ============================================================
void terminal_render_hook(BWE_Surface* surface) {
    if (!surface) return;

    // Find corresponding session by window ID (parent of this panel)
    ConsoleSession* session = conhost_get_session_by_window(surface->parent_id);
    if (!session) return;

    uint32_t text_color = 0xFF22D3EE; // Cyan 400
    const BVFontMetrics* font = BV_GetDefaultFont();
    uint32_t line_height = font ? font->line_height : 16;
    uint32_t char_width = font ? font->advance_x : 8;

    uint32_t visible_lines = (surface->screen_bounds.height > 12) 
                             ? (surface->screen_bounds.height - 12) / line_height 
                             : 1;

    // Auto-scroll calculation
    uint32_t start_line = 0;
    if (session->cursor_y >= visible_lines) {
        start_line = session->cursor_y - visible_lines + 1;
    }

    // Draw visible lines
    for (uint32_t i = 0; i < visible_lines; i++) {
        uint32_t buf_idx = start_line + i;
        if (buf_idx >= CONHOST_MAX_LINES) break;
        if (buf_idx > session->cursor_y) break;

        if (session->buffer[buf_idx][0] != '\0') {
            BOFont_DrawText(
                BOFont_GetRole(BOFONT_ROLE_MONO),
                session->buffer[buf_idx],
                surface->screen_bounds.x + 8,
                surface->screen_bounds.y + 6 + (int32_t)(i * line_height),
                text_color);
        }

        // Draw cursor caret on current cursor line
        if (buf_idx == session->cursor_y) {
            static uint32_t blink_counter = 0;
            blink_counter++;
            if ((blink_counter / 20) % 2 == 0) {
                int32_t caret_x = surface->screen_bounds.x + 8 + (int32_t)(session->cursor_x * char_width);
                int32_t caret_y = surface->screen_bounds.y + 6 + (int32_t)(i * line_height);
                BOFont_DrawText(BOFont_GetRole(BOFONT_ROLE_MONO), "_", caret_x, caret_y, 0xFFFFFFFF);
            }
        }
    }
}

// ============================================================
// Event Handler — Route GUI input into session FIFO
// ============================================================
void terminal_handle_event(uint32_t surface_id, const BVEvent* event) {
    if (!event) return;

    // Traverse up to find top-level window ID
    uint32_t curr = surface_id;
    BWE_Surface* s = BWE_GetSurface(curr);
    while (s && s->parent_id != BWE_DESKTOP_ID && s->parent_id != 0 && curr != s->parent_id) {
        curr = s->parent_id;
        s = BWE_GetSurface(curr);
    }

    ConsoleSession* session = conhost_get_session_by_window(curr);
    if (!session) return;

    if (event->type == BV_EVENT_KEY_DOWN || event->type == BV_EVENT_KEY_UP) {
        KeyboardEvent kevt;
        memset(&kevt, 0, sizeof(KeyboardEvent));
        kevt.keycode = event->key_code;
        kevt.ascii = event->ascii;
        kevt.pressed = (event->type == BV_EVENT_KEY_DOWN);
        kevt.shift = event->shift;
        kevt.ctrl = event->ctrl;
        kevt.alt = event->alt;
        kevt.caps_lock = event->caps_lock;

        conhost_push_key(session, &kevt);
    }
}

// ============================================================
// ConHost Integrated Lifecycle
// ============================================================
bwe_error_t terminal_init_for_session(void* session_ptr, const char* title) {
    ConsoleSession* session = (ConsoleSession*)session_ptr;
    if (!session) return BWE0001;

    uint32_t old_pid = g_current_creating_pid;
    g_current_creating_pid = (uint32_t)session->attached_pid;

    // Stagger window positions slightly based on session ID
    int32_t win_x = 150 + (int32_t)((session->session_id - 1) * 30);
    int32_t win_y = 100 + (int32_t)((session->session_id - 1) * 30);

    bwe_error_t err = BOS_CreateWindow(win_x, win_y, 560, 360, title ? title : "Terminal", &session->terminal_window_id);
    if (err != BWE_SUCCESS) {
        g_current_creating_pid = old_pid;
        return err;
    }

    err = BOS_CreatePanel(session->terminal_window_id, 0, 0, 560, 320, 0xFF0F172A, &session->terminal_body_id);
    if (err != BWE_SUCCESS) {
        g_current_creating_pid = old_pid;
        return err;
    }

    BWE_Surface* win = BWE_GetSurface(session->terminal_window_id);
    if (win) win->on_event = terminal_handle_event;

    BWE_Surface* panel = BWE_GetSurface(session->terminal_body_id);
    if (panel) {
        panel->on_event = terminal_handle_event;
        panel->on_render = terminal_render_hook;
    }

    BOS_SetFocus(session->terminal_window_id);

    g_current_creating_pid = old_pid;
    return BWE_SUCCESS;
}

// Legacy stub functions for standalone testing compatibility
bwe_error_t terminal_init(uint32_t* out_win) {
    (void)out_win;
    return BWE_SUCCESS;
}

void terminal_exit(void) {
}
