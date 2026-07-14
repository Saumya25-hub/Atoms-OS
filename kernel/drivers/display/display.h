#pragma once
#include <stdint.h>
#include <stdbool.h>

void display_init(void);
void display_print(const char* str);
void display_set_color(uint8_t color);
void display_clear(void);
void display_set_cursor(uint16_t x, uint16_t y);
void display_print_hex(uint64_t num);
void display_print_dec(uint64_t num);

// Runtime GUI Console Toggle
// When disabled (default after GUI starts), display_print() writes to serial only.
// When enabled, display_print() also draws text to the graphical framebuffer.
void display_gui_console_set_enabled(bool enabled);
bool display_gui_console_is_enabled(void);

