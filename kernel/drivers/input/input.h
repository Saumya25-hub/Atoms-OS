#ifndef KERNEL_INPUT_H
#define KERNEL_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "bovisual/Include/events.h"

#define MAX_EVENTS 1024

// Initialize the kernel input subsystem
void kernel_input_init(void);

// Push a raw mouse event from the PS/2 driver
void kernel_input_push_mouse(int32_t dx, int32_t dy, uint8_t buttons);

// Push a raw keyboard event from the PS/2 driver
void kernel_input_push_key(uint8_t scancode, bool is_pressed);

// Get the next event for BOVISUAL (Returns true if event populated)
bool kernel_get_event(BVEvent* out_event);

// Returns the number of events currently in the input queue
uint32_t kernel_input_get_queue_size(void);

#endif // KERNEL_INPUT_H
