#include "../libbos/include/bos.h"

// Very simple string length
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Very simple string compare
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void _start(void) {
    bos_print("======================================\n");
    bos_print("       BOS Interactive Shell          \n");
    bos_print("======================================\n");
    
    char input_buffer[128];
    int buf_idx = 0;

    while (1) {
        bos_print("\nBOS> ");
        buf_idx = 0;

        // Read line
        while (1) {
            char c = bos_getc();
            
            // Handle Enter
            if (c == '\n' || c == '\r') {
                bos_print("\n");
                input_buffer[buf_idx] = '\0';
                break;
            }
            
            // Handle Backspace
            if (c == '\b') {
                if (buf_idx > 0) {
                    buf_idx--;
                    // Print backspace, space, backspace to visually erase
                    bos_print("\b \b");
                }
                continue;
            }
            
            // Store normal character
            if (buf_idx < sizeof(input_buffer) - 1) {
                input_buffer[buf_idx++] = c;
                
                // Echo character
                char str[2] = {c, '\0'};
                bos_print(str);
            }
        }

        // Process Command
        if (buf_idx == 0) {
            continue;
        }

        if (strcmp(input_buffer, "help") == 0) {
            bos_print("Available commands:\n");
            bos_print("  help  - Show this message\n");
            bos_print("  clear - Clear the screen (not fully implemented, prints newlines)\n");
            bos_print("  exit  - Terminate the shell\n");
        } else if (strcmp(input_buffer, "clear") == 0) {
            for(int i=0; i<25; i++) bos_print("\n");
        } else if (strcmp(input_buffer, "exit") == 0) {
            bos_print("Exiting shell...\n");
            break;
        } else {
            bos_print("Unknown command: ");
            bos_print(input_buffer);
            bos_print("\n");
        }
    }

    bos_exit();
    while (1) { bos_yield(); }
}
