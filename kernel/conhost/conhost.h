#ifndef KERNEL_CONHOST_H
#define KERNEL_CONHOST_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/keyboard/include/keyboard.h"

#define CONHOST_MAX_SESSIONS 16
#define CONHOST_MAX_LINES 100
#define CONHOST_MAX_COLS  80
#define CONHOST_KBD_BUF_SIZE 64

typedef struct {
    bool active;
    uint32_t session_id;
    uint64_t attached_pid;       // PID of CLI process attached to this session
    uint32_t terminal_window_id; // BWE Window ID
    uint32_t terminal_body_id;   // BWE Body Panel ID

    char buffer[CONHOST_MAX_LINES][CONHOST_MAX_COLS];
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t scroll_offset;

    KeyboardEvent stdin_buffer[CONHOST_KBD_BUF_SIZE];
    volatile uint32_t stdin_head;
    volatile uint32_t stdin_tail;

    bool is_dirty;
} ConsoleSession;

// Subsystem lifecycle
void conhost_init(void);

// Session management
ConsoleSession* conhost_create_session(uint64_t pid);
ConsoleSession* conhost_get_session_by_pid(uint64_t pid);
ConsoleSession* conhost_get_session_by_window(uint32_t window_id);
ConsoleSession* conhost_get_session_by_id(uint32_t session_id);
ConsoleSession* conhost_get_active_session(void);
void conhost_destroy_session_by_pid(uint64_t pid);

// I/O Operations
bool conhost_write_pid(uint64_t pid, const char* str);
bool conhost_clear_pid(uint64_t pid);
bool conhost_set_cursor_pid(uint64_t pid, uint16_t x, uint16_t y);

// Keyboard FIFO
void conhost_push_key(ConsoleSession* session, const KeyboardEvent* evt);
bool conhost_pop_key_pid(uint64_t pid, KeyboardEvent* out_evt);

// Compositor integration
bool conhost_has_dirty_sessions(void);
void conhost_clear_dirty_all(void);

// Process spawn helper
int conhost_spawn_console_for_process(uint64_t pid, const char* app_name);

#endif // KERNEL_CONHOST_H
