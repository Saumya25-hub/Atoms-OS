#ifndef KERNEL_INPUT_ABSTRACTION_H
#define KERNEL_INPUT_ABSTRACTION_H

#include <stdint.h>
#include <stdbool.h>

// Unified Input Structure (Absolute position truth)
typedef struct {
    int32_t mouse_x;        // Absolute screen X
    int32_t mouse_y;        // Absolute screen Y
    uint8_t buttons;        // Button bitmask (Bit 0: Left, Bit 1: Right, Bit 2: Middle)
    int32_t scroll;         // Accumulated scroll delta
} InputState;

// Initialize the input abstraction layer normalizer
void input_abstraction_init(uint32_t screen_width, uint32_t screen_height);

// Update active screen resolution limits
void input_abstraction_update_resolution(uint32_t screen_width, uint32_t screen_height);

// Push absolute coordinates from primary input devices (e.g., USB Tablet)
void input_push_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons, int32_t scroll_delta);

// Push relative movement from legacy fallback input devices (e.g., PS/2 Mouse)
// Normalizer converts dx/dy into absolute coordinates internally
void input_push_relative(int32_t dx, int32_t dy, uint8_t buttons, int32_t scroll_delta);

// Read the unified input state truth snapshot (for BOHEART Engine)
InputState input_get_latest_state(void);

#endif // KERNEL_INPUT_ABSTRACTION_H
