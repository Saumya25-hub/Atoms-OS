#ifndef KERNEL_INPUT_CCTE_H
#define KERNEL_INPUT_CCTE_H

#include <stdint.h>

#define CCTE_VIRTUAL_MAX 65535

void ccte_init(void);

// Ingest from HIDA
void ccte_push_absolute(uint32_t backend_id, int32_t x, int32_t y, uint32_t max_x, uint32_t max_y, uint8_t buttons, int32_t scroll);
void ccte_push_relative(uint32_t backend_id, int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll);

// Dump Diagnostics
void ccte_dump_status(void);

#endif // KERNEL_INPUT_CCTE_H
