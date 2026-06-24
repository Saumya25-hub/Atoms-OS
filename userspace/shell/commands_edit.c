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

static void editor_render(const char* filename) {
    bos_clear_screen();
    
    // Print the buffer
    bos_print(editor_buf);
    
    // Calculate logical cursor x, y
    uint16_t x = 0;
    uint16_t y = 0;
    for (int i = 0; i < buf_len; i++) {
        if (editor_buf[i] == '\n') {
            x = 0;
            y++;
        } else if (editor_buf[i] == '\b') {
            if (x > 0) x--;
            else if (y > 0) { y--; x = 79; } // Rough approximation
        } else if (editor_buf[i] == '\t') {
            x = (x + 4) & ~3;
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
    
    // Enter keyboard loop
    bos_key_event_t evt;
    while (1) {
        editor_render(argv[1]);
        
        while (1) {
            bos_get_key_event(&evt);
            if (!evt.pressed) continue;
            
            if (evt.keycode == BOS_KEY_ESC) {
                bos_clear_screen();
                return;
            }
            
            if (evt.keycode == BOS_KEY_F1) {
                // Save to disk
                // First delete the old file if it exists, since our file engine creates fresh clusters
                // Or wait, bos_create truncates? Our bos_create in FAT32 just creates if it doesn't exist.
                // Wait, if it exists, bos_create might fail or not truncate.
                // Let's delete it first, then create.
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
            
            if (evt.ascii == '\b') {
                if (buf_len > 0) {
                    buf_len--;
                    editor_buf[buf_len] = '\0';
                    dirty = 1;
                }
                break; // Redraw
            }
            
            if (evt.ascii == '\n' || evt.ascii == '\r') {
                if (buf_len < MAX_FILE_SIZE - 1) {
                    editor_buf[buf_len++] = '\n';
                    editor_buf[buf_len] = '\0';
                    dirty = 1;
                }
                break; // Redraw
            }
            
            if (evt.ascii >= 32 && evt.ascii <= 126) {
                if (buf_len < MAX_FILE_SIZE - 1) {
                    editor_buf[buf_len++] = evt.ascii;
                    editor_buf[buf_len] = '\0';
                    dirty = 1;
                }
                break; // Redraw
            }
        }
    }
}

void commands_edit_init(void) {
    command_register("edit", cmd_edit, "Text Editor V1", "File");
}
