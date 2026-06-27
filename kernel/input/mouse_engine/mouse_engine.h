#ifndef MOUSE_ENGINE_H
#define MOUSE_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

void mouse_engine_init(uint32_t screen_width, uint32_t screen_height);
void mouse_engine_update_resolution(uint32_t screen_width, uint32_t screen_height);
void mouse_engine_push_packet(int32_t raw_dx, int32_t raw_dy, uint8_t buttons, bool overflow_x, bool overflow_y);

#endif
