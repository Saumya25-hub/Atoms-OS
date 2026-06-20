#pragma once
#include <stdint.h>

typedef struct {
    void (*init)(void);
    void (*draw_character)(uint16_t x, uint16_t y, char c, uint8_t color);
    void (*set_hardware_cursor)(uint16_t x, uint16_t y);
    void (*clear_memory)(uint8_t bg_color);
    uint16_t (*get_width)(void);
    uint16_t (*get_height)(void);
} BackendDriver;

void console_set_backend(BackendDriver* backend);
void console_draw_char(uint16_t x, uint16_t y, char c, uint8_t color);
void console_update_cursor(uint16_t x, uint16_t y);
void console_clear_all(uint8_t bg_color);
uint16_t console_get_width(void);
uint16_t console_get_height(void);
