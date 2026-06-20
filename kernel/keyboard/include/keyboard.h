#pragma once
#include <stdint.h>

// Interface for hardware keyboard drivers
typedef struct {
    void (*init)(void);
    uint8_t (*read_scancode)(void);
} KeyboardDriver;

// Initializes the manager, sets default driver internally, and requests IRQ1
void keyboard_init(void);

// Fetch character
char keyboard_get_char(void);

// Check if a key is pressed
uint8_t keyboard_key_pressed(void);

// Register a callback for when a key is pressed
void keyboard_register_callback(void (*callback)(char));
