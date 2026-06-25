#include "command.h"
#include "../libbos/include/bos.h"
#include "../libbos/include/bodiskhub.h"

#define MAX_FILE_SIZE 8192
static char editor_buf[MAX_FILE_SIZE];
static int buf_len = 0;
static int dirty = 0;

static void print_dec(uint32_t num) {
    if (num == 0) { bos_print("0"); return; }
    char buf[16]; int i = 14; buf[15] = '\0';
    while (num > 0) { buf[i--] = (num % 10) + '0'; num /= 10; }
    bos_print(&buf[i + 1]);
}

static void resolve_absolute_path(const char* input, char* out) {
    if (input[0] == '/') {
        int i = 0; while (input[i] && i < 255) { out[i] = input[i]; i++; } out[i] = '\0';
        return;
    }
    const char* current_path = commands_bodh_get_cwd();
    int i = 0; while (current_path[i] && i < 255) { out[i] = current_path[i]; i++; }
    if (i > 0 && out[i-1] != '/') out[i++] = '/';
    int j = 0; while (input[j] && i < 255) { out[i++] = input[j++]; }
    out[i] = '\0';
}

static void editor_render(const char* filename, int cursor_idx) {
    bos_clear_screen();
    
    // Print the buffer
    bos_print(editor_buf);
    
    // Calculate logical cursor x, y for the cursor_idx
    uint16_t x = 0;
    uint16_t y = 0;
    for (int i = 0; i < cursor_idx; i++) {
        if (editor_buf[i] == '\n') {
            x = 0;
            y++;
        } else if (editor_buf[i] == '\t') {
            x = (x + 4) & ~3;
            if (x >= 80) { x = 0; y++; }
        } else {
            x++;
            if (x >= 80) { x = 0; y++; }
        }
    }
    
    // Print the status bar at bottom (row 23 and 24)
    bos_set_cursor(0, 23);
    bos_print("--------------------------------------------------------------------------------");
    bos_set_cursor(0, 24);
    bos_print(filename);
    if (dirty) bos_print(" *");
    bos_print(" | Size: ");
    print_dec(buf_len);
    bos_print(" Bytes | [F1] Save | [ESC] Exit");
    
    // Restore hardware cursor to the logical typing position
    bos_set_cursor(x, y);
}

static void cmd_edit(int argc, char** argv) {
    if (argc < 2) { bos_print("Usage: edit <filename>\n"); return; }
    
    char abs_path[256];
    resolve_absolute_path(argv[1], abs_path);
    
    // Load existing file or clear buffer
    buf_len = 0;
    dirty = 0;
    
    int fd = bos_open(abs_path);
    if (fd >= 0) {
        buf_len = bos_read(fd, editor_buf, MAX_FILE_SIZE - 1);
        if (buf_len < 0) buf_len = 0; // Read error fallback
        bos_close(fd);
    }
    editor_buf[buf_len] = '\0';
    
    int cursor_idx = buf_len;
    
    // Enter keyboard loop
    bos_key_event_t evt;
    while (1) {
        editor_render(argv[1], cursor_idx);
        
        while (1) {
            bos_get_key_event(&evt);
            if (!evt.pressed) continue;
            
            if (evt.keycode == BOS_KEY_ESC) {
                bos_clear_screen();
                return;
            }
            
            if (evt.keycode == BOS_KEY_F1) {
                bodh_object_delete(abs_path);
                if (bodh_file_create(abs_path) == 0) {
                    int wfd = bos_open(abs_path);
                    if (wfd >= 0) {
                        bos_write(wfd, editor_buf, buf_len);
                        bos_close(wfd);
                        dirty = 0;
                    }
                }
                break; // Redraw
            }
            
            if (evt.keycode == BOS_KEY_LEFT) {
                if (cursor_idx > 0) cursor_idx--;
                break;
            }
            if (evt.keycode == BOS_KEY_RIGHT) {
                if (cursor_idx < buf_len) cursor_idx++;
                break;
            }
            if (evt.keycode == BOS_KEY_UP) {
                uint16_t curr_x = 0;
                int start_of_line = cursor_idx;
                while (start_of_line > 0 && editor_buf[start_of_line - 1] != '\n') {
                    start_of_line--;
                }
                curr_x = cursor_idx - start_of_line;

                if (start_of_line > 0) {
                    int prev_line_start = start_of_line - 1;
                    while (prev_line_start > 0 && editor_buf[prev_line_start - 1] != '\n') {
                        prev_line_start--;
                    }
                    int prev_line_len = (start_of_line - 1) - prev_line_start;
                    if (curr_x > prev_line_len) curr_x = prev_line_len;
                    cursor_idx = prev_line_start + curr_x;
                }
                break;
            }
            if (evt.keycode == BOS_KEY_DOWN) {
                uint16_t curr_x = 0;
                int start_of_line = cursor_idx;
                while (start_of_line > 0 && editor_buf[start_of_line - 1] != '\n') {
                    start_of_line--;
                }
                curr_x = cursor_idx - start_of_line;

                int next_line_start = cursor_idx;
                while (next_line_start < buf_len && editor_buf[next_line_start] != '\n') {
                    next_line_start++;
                }
                if (next_line_start < buf_len) {
                    next_line_start++; // skip \n
                    int next_line_end = next_line_start;
                    while (next_line_end < buf_len && editor_buf[next_line_end] != '\n') {
                        next_line_end++;
                    }
                    int next_line_len = next_line_end - next_line_start;
                    if (curr_x > next_line_len) curr_x = next_line_len;
                    cursor_idx = next_line_start + curr_x;
                }
                break;
            }
            
            if (evt.ascii == '\b') { // Backspace
                if (cursor_idx > 0) {
                    for (int i = cursor_idx; i <= buf_len; i++) {
                        editor_buf[i - 1] = editor_buf[i];
                    }
                    buf_len--;
                    cursor_idx--;
                    dirty = 1;
                }
                break;
            }
            if (evt.keycode == BOS_KEY_DEL) { // Delete
                if (cursor_idx < buf_len) {
                    for (int i = cursor_idx + 1; i <= buf_len; i++) {
                        editor_buf[i - 1] = editor_buf[i];
                    }
                    buf_len--;
                    dirty = 1;
                }
                break;
            }
            
            if (evt.ascii == '\n' || evt.ascii == '\r') {
                if (buf_len < MAX_FILE_SIZE - 1) {
                    for (int i = buf_len; i >= cursor_idx; i--) {
                        editor_buf[i + 1] = editor_buf[i];
                    }
                    editor_buf[cursor_idx] = '\n';
                    buf_len++;
                    cursor_idx++;
                    dirty = 1;
                }
                break;
            }
            
            if (evt.ascii >= 32 && evt.ascii <= 126) {
                if (buf_len < MAX_FILE_SIZE - 1) {
                    for (int i = buf_len; i >= cursor_idx; i--) {
                        editor_buf[i + 1] = editor_buf[i];
                    }
                    editor_buf[cursor_idx] = evt.ascii;
                    buf_len++;
                    cursor_idx++;
                    dirty = 1;
                }
                break;
            }
        }
    }
}

void commands_edit_init(void) {
    command_register("edit", cmd_edit, "Text Editor V1", "File");
}
