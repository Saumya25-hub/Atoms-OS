#include "vga.h"
#include "arch/x86_64/io/port_io.h"

#define VGA_MEMORY (uint16_t*)0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// VGA CRT Controller Registers
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA  0x3D5
#define CRTC_CURSOR_HIGH 0x0E
#define CRTC_CURSOR_LOW  0x0F

void vga_init(void) {
    // Basic initialization for VGA text mode (assumes BIOS already set it up)
}

void vga_draw_character(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    volatile uint16_t* buffer = VGA_MEMORY;
    uint16_t index = y * VGA_WIDTH + x;
    buffer[index] = (uint16_t)c | ((uint16_t)color << 8);
}

void vga_set_hardware_cursor(uint16_t x, uint16_t y) {
    if (x >= VGA_WIDTH || y >= VGA_HEIGHT) return;
    uint16_t pos = y * VGA_WIDTH + x;
    
    io_out8(VGA_CRTC_INDEX, CRTC_CURSOR_LOW);
    io_out8(VGA_CRTC_DATA, (uint8_t)(pos & 0xFF));
    io_out8(VGA_CRTC_INDEX, CRTC_CURSOR_HIGH);
    io_out8(VGA_CRTC_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_clear_memory(uint8_t bg_color) {
    uint16_t blank = 0x20 | ((uint16_t)bg_color << 8); // Space character
    volatile uint16_t* buffer = VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        buffer[i] = blank;
    }
}

uint16_t vga_get_screen_width(void) {
    return VGA_WIDTH;
}

uint16_t vga_get_screen_height(void) {
    return VGA_HEIGHT;
}
