#pragma once
#include <stdint.h>

void io_out8(uint16_t port, uint8_t data);
uint8_t io_in8(uint16_t port);
void io_out16(uint16_t port, uint16_t data);
uint16_t io_in16(uint16_t port);
void io_out32(uint16_t port, uint32_t data);
uint32_t io_in32(uint16_t port);
