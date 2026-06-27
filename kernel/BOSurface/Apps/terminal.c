#include "terminal.h"
#include "kernel/BOSurface/Core/app_manager.h"
#include "bovisual/Include/text.h"
#include "bovisual/Include/graphics.h"
#include "kernel/keyboard/include/keyboard.h"
#include "kernel/lib/include/string.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/display/display.h"

#define TERM_MAX_LINES 100
#define TERM_MAX_COLS  80

// --- Inline helpers (strcat/strncpy not in kernel string.h) ---
static void term_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static void term_strncpy(char* dest, const char* src, uint32_t n) {
    uint32_t i = 0;
    while (i < n && src[i]) { dest[i] = src[i]; i++; }
    while (i < n) { dest[i] = '\0'; i++; }
}

// --- Terminal Context ---
typedef struct {
    uint32_t window_id;
    uint32_t body_id;

    char buffer[TERM_MAX_LINES][TERM_MAX_COLS];
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t scroll_offset;

    char current_line[TERM_MAX_COLS];
    uint32_t current_len;

    bool shift_pressed;
    bool caps_lock;

    uint32_t blink_counter;
    bool show_caret;
} BOS_TerminalContext;

static BOS_TerminalContext* term_ctx = 0;

// --- Scancode tables (local to terminal, no keyboard driver modification) ---
static const char term_sc_normal[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

static const char term_sc_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0
};

static void terminal_print(const char* str);
static void terminal_execute_command(void);

// ============================================================
// Render Hook — Draw terminal text on the body panel surface
// ============================================================
void terminal_render_hook(BWE_Surface* surface) {
    if (!term_ctx || !surface) return;

    uint32_t text_color = 0xFF22D3EE; // Cyan 400
    const BVFontMetrics* font = BV_GetDefaultFont();
    uint32_t line_height = font ? font->line_height : 16;
    // Account for 6px top padding and 6px bottom padding in the calculation
    uint32_t visible_lines = (surface->screen_bounds.height > 12) 
                             ? (surface->screen_bounds.height - 12) / line_height 
                             : 1;

    // Auto-scroll: keep cursor_y visible
    uint32_t start_line = 0;
    if (term_ctx->cursor_y >= visible_lines) {
        start_line = term_ctx->cursor_y - visible_lines + 1;
    }

    // Draw scrollback lines
    for (uint32_t i = 0; i < visible_lines; i++) {
        uint32_t buf_idx = start_line + i;
        if (buf_idx >= TERM_MAX_LINES) break;
        if (buf_idx >= term_ctx->cursor_y) break; // Don't draw past cursor

        if (term_ctx->buffer[buf_idx][0] != '\0') {
            BOVISUAL_Draw_String(
                surface->screen_bounds.x + 8,
                surface->screen_bounds.y + 6 + (int32_t)(i * line_height),
                term_ctx->buffer[buf_idx],
                text_color, 0, true, font);
        }
    }

    // Draw current input line (prompt + typed text + caret)
    uint32_t current_vis_y = term_ctx->cursor_y - start_line;
    if (current_vis_y < visible_lines) {
        char prompt_buf[TERM_MAX_COLS];
        strcpy(prompt_buf, "root@signatures:~$ ");
        term_strcat(prompt_buf, term_ctx->current_line);

        // Blinking caret
        term_ctx->blink_counter++;
        if (term_ctx->blink_counter > 30) {
            term_ctx->show_caret = !term_ctx->show_caret;
            term_ctx->blink_counter = 0;
        }
        if (term_ctx->show_caret) {
            term_strcat(prompt_buf, "_");
        }

        BOVISUAL_Draw_String(
            surface->screen_bounds.x + 8,
            surface->screen_bounds.y + 6 + (int32_t)(current_vis_y * line_height),
            prompt_buf,
            text_color, 0, true, font);
    }
}

// ============================================================
// Event Handler — Keyboard input
// ============================================================
void terminal_handle_event(uint32_t surface_id, const BVEvent* event) {
    (void)surface_id;
    if (!term_ctx) return;

    if (event->type == BV_EVENT_KEY_DOWN) {
        uint8_t sc = event->key_code;

        // Backspace ('\b' is 8)
        if (sc == '\b') {
            if (term_ctx->current_len > 0) {
                term_ctx->current_len--;
                term_ctx->current_line[term_ctx->current_len] = '\0';
            }
            return;
        }

        // Enter ('\n' is 10)
        if (sc == '\n') {
            terminal_execute_command();
            return;
        }

        // Printable ASCII character input
        if (sc >= ' ' && sc <= '~') {
            if (term_ctx->current_len < TERM_MAX_COLS - 22) {
                term_ctx->current_line[term_ctx->current_len++] = (char)sc;
                term_ctx->current_line[term_ctx->current_len] = '\0';
            }
        }
    }
    // We don't need to handle KEY_UP anymore since the driver handles shift parsing
}

// ============================================================
// terminal_print — Push a line into scrollback buffer
// ============================================================
static void terminal_print(const char* str) {
    if (!term_ctx) return;

    term_strncpy(term_ctx->buffer[term_ctx->cursor_y], str, TERM_MAX_COLS - 1);
    term_ctx->buffer[term_ctx->cursor_y][TERM_MAX_COLS - 1] = '\0';

    term_ctx->cursor_y++;
    if (term_ctx->cursor_y >= TERM_MAX_LINES) {
        // Scroll entire buffer up by 1
        for (uint32_t i = 1; i < TERM_MAX_LINES; i++) {
            strcpy(term_ctx->buffer[i - 1], term_ctx->buffer[i]);
        }
        term_ctx->buffer[TERM_MAX_LINES - 1][0] = '\0';
        term_ctx->cursor_y = TERM_MAX_LINES - 1;
    }
}

// ============================================================
// Command Parser
// ============================================================
static void terminal_execute_command(void) {
    if (!term_ctx) return;

    // Echo the prompt + typed command into scrollback
    char echo_buf[TERM_MAX_COLS];
    strcpy(echo_buf, "root@signatures:~$ ");
    term_strcat(echo_buf, term_ctx->current_line);
    terminal_print(echo_buf);

    // Parse
    if (strlen(term_ctx->current_line) > 0) {
        char* cmd = term_ctx->current_line;

        if (strcmp(cmd, "help") == 0) {
            terminal_print("Available commands:");
            terminal_print("  help     - Show this message");
            terminal_print("  clear    - Clear screen");
            terminal_print("  cls      - Clear screen");
            terminal_print("  ver      - Show OS version");
            terminal_print("  about    - About SignaturesOS");
            terminal_print("  echo     - Print text");
            terminal_print("  sysinfo  - System information");
            terminal_print("  mem      - Memory info");
            terminal_print("  ls       - List files");
            terminal_print("  pwd      - Print directory");
            terminal_print("  time     - Show time");
            terminal_print("  date     - Show date");
        }
        else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
            for (uint32_t i = 0; i < TERM_MAX_LINES; i++) {
                term_ctx->buffer[i][0] = '\0';
            }
            term_ctx->cursor_y = 0;
            term_ctx->scroll_offset = 0;
        }
        else if (strcmp(cmd, "ver") == 0) {
            terminal_print("SignaturesOS v0.10 (Phase 10 Terminal Engine)");
            terminal_print("BISHOP Windowing Engine v2.0");
        }
        else if (strcmp(cmd, "about") == 0) {
            terminal_print("SignaturesOS - Custom Operating System");
            terminal_print("Built from scratch. No Linux. No POSIX.");
            terminal_print("Architecture: x86_64 Long Mode");
            terminal_print("Display: BOSurface + BOVISUAL");
        }
        else if (strncmp(cmd, "echo ", 5) == 0) {
            terminal_print(cmd + 5);
        }
        else if (strcmp(cmd, "echo") == 0) {
            terminal_print("");
        }
        else if (strcmp(cmd, "sysinfo") == 0) {
            terminal_print("CPU: x86_64 (Long Mode)");
            terminal_print("RAM: 64MB (QEMU)");
            terminal_print("Display: 800x600x32bpp VBE");
            terminal_print("Shell: BOSurface Desktop");
        }
        else if (strcmp(cmd, "mem") == 0) {
            terminal_print("Memory: 64MB Total (QEMU)");
            terminal_print("Heap: Kernel bump allocator active");
        }
        else if (strcmp(cmd, "ls") == 0) {
            extern int vfs_readdir(const char* path, int index, void* out_entry);
            // vfs_dirent_t: char name[64]; uint32_t size; uint8_t is_directory; uint32_t cluster;
            uint8_t entry_buf[76]; // sizeof(vfs_dirent_t) — name[64]+size[4]+is_dir[1]+cluster[4]+padding
            int idx = 0;
            int found = 0;
            while (vfs_readdir("/", idx, entry_buf) == 0) {
                char* name = (char*)entry_buf;
                uint8_t is_dir = entry_buf[68]; // offset of is_directory
                if (name[0] != '\0') {
                    char line[TERM_MAX_COLS];
                    if (is_dir) {
                        strcpy(line, "  [DIR]  ");
                    } else {
                        strcpy(line, "  [FILE] ");
                    }
                    term_strcat(line, name);
                    terminal_print(line);
                    found++;
                }
                idx++;
            }
            if (found == 0) {
                terminal_print("(empty directory)");
            }
        }
        else if (strcmp(cmd, "pwd") == 0) {
            terminal_print("/root");
        }
        else if (strcmp(cmd, "time") == 0) {
            terminal_print("00:00:00 (RTC not implemented yet)");
        }
        else if (strcmp(cmd, "date") == 0) {
            terminal_print("2026-06-27 (RTC not implemented yet)");
        }
        else {
            char err_buf[TERM_MAX_COLS];
            strcpy(err_buf, "Unknown command: ");
            term_strcat(err_buf, cmd);
            terminal_print(err_buf);
        }
    }

    // Reset input line
    term_ctx->current_line[0] = '\0';
    term_ctx->current_len = 0;
    term_ctx->cursor_x = 0;
}

// ============================================================
// Application Init/Exit (called by Application Manager)
// ============================================================
bwe_error_t terminal_init(uint32_t* out_win) {
    if (term_ctx != 0) return BWE0008; // Already running

    term_ctx = (BOS_TerminalContext*)kmalloc(sizeof(BOS_TerminalContext));
    if (!term_ctx) return BWE0004; // No memory

    memset(term_ctx, 0, sizeof(BOS_TerminalContext));

    bwe_error_t err = BOS_CreateWindow(200, 120, 500, 350, "Terminal (tty1)", &term_ctx->window_id);
    if (err != BWE_SUCCESS) {
        kfree(term_ctx);
        term_ctx = 0;
        return err;
    }

    // Dark terminal body panel
    err = BOS_CreatePanel(term_ctx->window_id, 0, 30, 500, 320, 0xFF0F172A, &term_ctx->body_id);
    if (err != BWE_SUCCESS) {
        kfree(term_ctx);
        term_ctx = 0;
        return err;
    }

    // Attach hooks to window and panel
    BWE_Surface* win = BWE_GetSurface(term_ctx->window_id);
    if (win) {
        win->on_event = terminal_handle_event;
    }

    BWE_Surface* panel = BWE_GetSurface(term_ctx->body_id);
    if (panel) {
        panel->on_event = terminal_handle_event;
        panel->on_render = terminal_render_hook;
    }

    // Welcome message
    terminal_print("SignaturesOS Terminal v0.10");
    terminal_print("Type 'help' for available commands.");
    terminal_print("");

    if (out_win) *out_win = term_ctx->window_id;
    return BWE_SUCCESS;
}

void terminal_exit(void) {
    if (term_ctx) {
        display_print("[APP] Terminal: Session closed, context freed\n");
        kfree(term_ctx);
        term_ctx = 0;
    }
}
