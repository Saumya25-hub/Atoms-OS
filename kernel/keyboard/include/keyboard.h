#pragma once
#include <stdint.h>

#include <stdbool.h>

// Represents a single keyboard event (press or release)
typedef struct {
    uint8_t scancode;
    char ascii;
    bool pressed;
    bool shift;
    bool ctrl;
    bool alt;
} KeyboardEvent;

// Interface for hardware keyboard drivers
typedef struct {
    void (*init)(void);
    uint8_t (*read_scancode)(void);
} KeyboardDriver;

// Initializes the manager, sets default driver internally, and requests IRQ1
void keyboard_init(void);

// Register a callback for when a key event occurs
void keyboard_register_callback(void (*callback)(KeyboardEvent* event));
char keyboard_getc(void);
