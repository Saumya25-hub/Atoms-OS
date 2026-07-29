#pragma once
#include <stdint.h>
#include <stdbool.h>

// Initialize VMMouse driver via VMware backdoor.
// Returns true if VMMouse is detected and absolute mode is enabled.
bool vmmouse_init(uint32_t screen_width, uint32_t screen_height);

// Check if VMMouse is currently active
bool vmmouse_is_active(void);

// Read absolute mouse position from VMMouse backdoor.
// Should be called periodically (e.g., on PS/2 IRQ or timer).
// Returns true if a packet was successfully read.
bool vmmouse_read(int32_t* abs_x, int32_t* abs_y, uint8_t* buttons);

// Poll VMMouse backdoor for pending absolute packets and push to HIDA.
// Called from the kernel main loop and from PS/2 IRQ12 handler when VMMouse is active.
void vmmouse_poll(void);
