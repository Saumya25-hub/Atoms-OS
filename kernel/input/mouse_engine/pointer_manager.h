#ifndef POINTER_MANAGER_H
#define POINTER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

void pointer_manager_init(uint32_t initial_x, uint32_t initial_y);
void pointer_manager_apply_movement(int32_t dx, int32_t dy, uint8_t buttons);

#endif
