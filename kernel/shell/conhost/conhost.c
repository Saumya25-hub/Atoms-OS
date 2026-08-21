#include "conhost.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/drivers/display/display.h"

static ConsoleSession g_sessions[CONHOST_MAX_SESSIONS];
extern uint32_t BOS_GetActiveSurface(void);
extern void scheduler_yield(void);

void conhost_init(void) {
    memset(g_sessions, 0, sizeof(g_sessions));
    display_print("[CONHOST] Console Host Subsystem Initialized (16 Sessions Max)\n");
}

ConsoleSession* conhost_create_session(uint64_t pid) {
    // Check if process already has a session
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active && g_sessions[i].attached_pid == pid) {
            return &g_sessions[i];
        }
    }

    // Find free slot
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (!g_sessions[i].active) {
            memset(&g_sessions[i], 0, sizeof(ConsoleSession));
            g_sessions[i].active = true;
            g_sessions[i].session_id = i + 1;
            g_sessions[i].attached_pid = pid;
            g_sessions[i].is_dirty = true;
            return &g_sessions[i];
        }
    }
    display_print("[CONHOST] ERROR: Max console sessions reached\n");
    return 0;
}

ConsoleSession* conhost_get_session_by_pid(uint64_t pid) {
    if (pid == 0) return 0;
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active && g_sessions[i].attached_pid == pid) {
            return &g_sessions[i];
        }
    }
    return 0;
}

ConsoleSession* conhost_get_session_by_window(uint32_t window_id) {
    if (window_id == 0) return 0;
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active && g_sessions[i].terminal_window_id == window_id) {
            return &g_sessions[i];
        }
    }
    return 0;
}

ConsoleSession* conhost_get_session_by_id(uint32_t session_id) {
    if (session_id == 0 || session_id > CONHOST_MAX_SESSIONS) return 0;
    if (g_sessions[session_id - 1].active && g_sessions[session_id - 1].session_id == session_id) {
        return &g_sessions[session_id - 1];
    }
    return 0;
}

ConsoleSession* conhost_get_active_session(void) {
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active) {
            return &g_sessions[i];
        }
    }
    return 0;
}

void conhost_destroy_session_by_pid(uint64_t pid) {
    if (pid == 0) return;
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active && g_sessions[i].attached_pid == pid) {
            display_print("[CONHOST] Destroying console session for PID ");
            extern void display_print_dec(uint64_t n);
            display_print_dec(pid);
            display_print("\n");
            g_sessions[i].active = false;
            g_sessions[i].attached_pid = 0;
            g_sessions[i].terminal_window_id = 0;
            g_sessions[i].terminal_body_id = 0;
            return;
        }
    }
}

static void scroll_session(ConsoleSession* session) {
    for (uint32_t i = 0; i < CONHOST_MAX_LINES - 1; i++) {
        memcpy(session->buffer[i], session->buffer[i + 1], CONHOST_MAX_COLS);
    }
    memset(session->buffer[CONHOST_MAX_LINES - 1], 0, CONHOST_MAX_COLS);
    if (session->cursor_y > 0) session->cursor_y--;
}

void conhost_write(ConsoleSession* session, const char* str) {
    if (!session || !str) return;

    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (c == '\n') {
            session->cursor_x = 0;
            session->cursor_y++;
        } else if (c == '\r') {
            session->cursor_x = 0;
        } else if (c == '\b') {
            if (session->cursor_x > 0) {
                session->cursor_x--;
            }
        } else if (c == '\t') {
            uint32_t spaces = 4 - (session->cursor_x % 4);
            for (uint32_t s = 0; s < spaces; s++) {
                if (session->cursor_x < CONHOST_MAX_COLS - 1) {
                    if (session->buffer[session->cursor_y][session->cursor_x] == '\0') {
                        session->buffer[session->cursor_y][session->cursor_x + 1] = '\0';
                    }
                    session->buffer[session->cursor_y][session->cursor_x] = ' ';
                    session->cursor_x++;
                }
            }
        } else {
            // Fill any intermediate gaps with spaces if cursor jumped
            uint32_t len = (uint32_t)strlen(session->buffer[session->cursor_y]);
            while (len < session->cursor_x && len < CONHOST_MAX_COLS - 1) {
                session->buffer[session->cursor_y][len] = ' ';
                session->buffer[session->cursor_y][len + 1] = '\0';
                len++;
            }

            bool was_end = (session->buffer[session->cursor_y][session->cursor_x] == '\0');
            session->buffer[session->cursor_y][session->cursor_x] = c;
            session->cursor_x++;
            if (session->cursor_x >= CONHOST_MAX_COLS - 1) {
                session->buffer[session->cursor_y][CONHOST_MAX_COLS - 1] = '\0';
                session->cursor_x = 0;
                session->cursor_y++;
            } else if (was_end) {
                session->buffer[session->cursor_y][session->cursor_x] = '\0';
            }
        }

        while (session->cursor_y >= CONHOST_MAX_LINES) {
            scroll_session(session);
        }
    }
    session->is_dirty = true;
}

bool conhost_write_pid(uint64_t pid, const char* str) {
    ConsoleSession* session = conhost_get_session_by_pid(pid);
    if (session) {
        conhost_write(session, str);
        return true;
    }
    return false;
}

bool conhost_clear_pid(uint64_t pid) {
    ConsoleSession* session = conhost_get_session_by_pid(pid);
    if (session) {
        for (uint32_t i = 0; i < CONHOST_MAX_LINES; i++) {
            memset(session->buffer[i], 0, CONHOST_MAX_COLS);
        }
        session->cursor_x = 0;
        session->cursor_y = 0;
        session->scroll_offset = 0;
        session->is_dirty = true;
        return true;
    }
    return false;
}

bool conhost_set_cursor_pid(uint64_t pid, uint16_t x, uint16_t y) {
    ConsoleSession* session = conhost_get_session_by_pid(pid);
    if (session) {
        if (x < CONHOST_MAX_COLS) session->cursor_x = x;
        if (y < CONHOST_MAX_LINES) session->cursor_y = y;
        session->is_dirty = true;
        return true;
    }
    return false;
}

void conhost_push_key(ConsoleSession* session, const KeyboardEvent* evt) {
    if (!session || !evt) return;
    uint32_t next_head = (session->stdin_head + 1) % CONHOST_KBD_BUF_SIZE;
    if (next_head != session->stdin_tail) {
        session->stdin_buffer[session->stdin_head] = *evt;
        session->stdin_head = next_head;
    }
}

#include "kernel/core/interrupt/include/irq_flags.h"

bool conhost_pop_key_pid(uint64_t pid, KeyboardEvent* out_evt) {
    ConsoleSession* session = conhost_get_session_by_pid(pid);
    if (!session || !out_evt) return false;

    irq_flags_t flags = irq_save();
    if (session->stdin_tail == session->stdin_head) {
        irq_restore(flags);
        return false;
    }
    *out_evt = session->stdin_buffer[session->stdin_tail];
    session->stdin_tail = (session->stdin_tail + 1) % CONHOST_KBD_BUF_SIZE;
    irq_restore(flags);
    return true;
}

bool conhost_has_dirty_sessions(void) {
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active && g_sessions[i].is_dirty) {
            return true;
        }
    }
    return false;
}

void conhost_clear_dirty_all(void) {
    for (int i = 0; i < CONHOST_MAX_SESSIONS; i++) {
        if (g_sessions[i].active) {
            g_sessions[i].is_dirty = false;
        }
    }
}

extern uint32_t terminal_init_for_session(ConsoleSession* session, const char* title);

int conhost_spawn_console_for_process(uint64_t pid, const char* app_name) {
    ConsoleSession* session = conhost_create_session(pid);
    if (!session) return -1;

    char title[64];
    strcpy(title, "Terminal - ");
    int len = (int)strlen(title);
    int app_len = (int)strlen(app_name);
    for (int i = 0; i < app_len && len < 60; i++) {
        title[len++] = app_name[i];
    }
    title[len] = '\0';

    uint32_t err = terminal_init_for_session(session, title);
    if (err != 0) {
        display_print("[CONHOST] ERROR: Failed to launch Terminal GUI for session\n");
        conhost_destroy_session_by_pid(pid);
        return -1;
    }
    display_print("[CONHOST] Attached PID ");
    extern void display_print_dec(uint64_t n);
    display_print_dec(pid);
    display_print(" to Console Session #");
    display_print_dec(session->session_id);
    display_print("\n");
    return 0;
}
