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
