#pragma once
#include <stdint.h>

void vga_init(void);
void vga_draw_character(uint16_t x, uint16_t y, char c, uint8_t color);
void vga_set_hardware_cursor(uint16_t x, uint16_t y);
void vga_clear_memory(uint8_t bg_color);
uint16_t vga_get_screen_width(void);
uint16_t vga_get_screen_height(void);
