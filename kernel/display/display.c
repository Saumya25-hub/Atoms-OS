#include "display.h"
#include "kernel/console/console.h"

static uint16_t current_row = 0;
static uint16_t current_col = 0;
static uint8_t current_color = 0x0F;

static void scroll_if_needed(void) {
    uint16_t height = console_get_height();
    if (height == 0) return;

    if (current_row >= height) {
        // For Phase 6 V1, wrap to top and clear when scrolling is needed
        current_row = 0;
        current_col = 0;
        console_clear_all(0x00);
    }
}

static void handle_newline(void) {
    current_col = 0;
    current_row++;
    scroll_if_needed();
}

void display_init(void) {
    current_row = 0;
    current_col = 0;
    current_color = 0x0F;
}

void display_print(const char* str) {
    uint16_t width = console_get_width();
    if (width == 0) return;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            handle_newline();
        } else if (str[i] == '\t') {
            current_col = (current_col + 4) & ~3;
            if (current_col >= width) handle_newline();
        } else if (str[i] == '\b') {
            if (current_col > 0) {
                current_col--;
            } else if (current_row > 0) {
                current_row--;
                current_col = width - 1;
            }
        } else {
            console_draw_char(current_col, current_row, str[i], current_color);
            current_col++;
            if (current_col >= width) {
                handle_newline();
            }
        }
    }
    console_update_cursor(current_col, current_row);
}

void display_set_color(uint8_t color) {
    current_color = color;
}

void display_clear(void) {
    console_clear_all(0x00);
    current_row = 0;
    current_col = 0;
    console_update_cursor(current_col, current_row);
}

void display_print_hex(uint64_t num) {
    display_print("0x");
    if (num == 0) {
        display_print("0");
        return;
    }
    
    char buffer[17];
    int i = 15;
    buffer[16] = '\0';
    
    while (num > 0 && i >= 0) {
        uint8_t rem = num % 16;
        if (rem < 10) buffer[i] = '0' + rem;
        else buffer[i] = 'A' + (rem - 10);
        num /= 16;
        i--;
    }
    
    display_print(&buffer[i + 1]);
}

void display_print_dec(uint64_t num) {
    if (num == 0) {
        display_print("0");
        return;
    }
    
    char buffer[21];
    int i = 19;
    buffer[20] = '\0';
    
    while (num > 0 && i >= 0) {
        buffer[i] = '0' + (num % 10);
        num /= 10;
        i--;
    }
    
    display_print(&buffer[i + 1]);
}
