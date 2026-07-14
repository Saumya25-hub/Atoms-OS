#pragma once
#include <stdint.h>
#include <stdbool.h>

// Initialize VMMouse driver via VMware backdoor.
// Returns true if VMMouse is detected and absolute mode is enabled.
bool vmmouse_init(uint32_t screen_width, uint32_t screen_height);
void vmmouse_update_resolution(uint32_t screen_width, uint32_t screen_height);
void vmmouse_get_bounds(uint32_t* out_w, uint32_t* out_h);

// Check if VMMouse is currently active
bool vmmouse_is_active(void);

// Read absolute mouse position from VMMouse backdoor.
// Should be called periodically (e.g., on PS/2 IRQ or timer).
// Returns true if a packet was successfully read.
bool vmmouse_read(int32_t* abs_x, int32_t* abs_y, uint8_t* buttons);
