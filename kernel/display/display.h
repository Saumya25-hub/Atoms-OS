#pragma once
#include <stdint.h>

void display_init(void);
void display_print(const char* str);
void display_set_color(uint8_t color);
void display_clear(void);
void display_print_hex(uint64_t num);
void display_print_dec(uint64_t num);

