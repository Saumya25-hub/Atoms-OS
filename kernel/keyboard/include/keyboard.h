#pragma once
#include <stdint.h>

#include <stdbool.h>

#define BOS_KEY_UP    0x80
#define BOS_KEY_DOWN  0x81
#define BOS_KEY_LEFT  0x82
#define BOS_KEY_RIGHT 0x83
#define BOS_KEY_ESC   0x84
#define BOS_KEY_F1    0x85
#define BOS_KEY_F2    0x86
#define BOS_KEY_F3    0x87
#define BOS_KEY_F4    0x88
#define BOS_KEY_F5    0x89
#define BOS_KEY_F6    0x8A
#define BOS_KEY_F7    0x8B
#define BOS_KEY_F8    0x8C
#define BOS_KEY_F9    0x8D
#define BOS_KEY_F10   0x8E
#define BOS_KEY_F11   0x8F
#define BOS_KEY_F12   0x90
#define BOS_KEY_HOME  0x91
#define BOS_KEY_END   0x92
#define BOS_KEY_PGUP  0x93
#define BOS_KEY_PGDN  0x94
#define BOS_KEY_INS   0x95
#define BOS_KEY_DEL   0x96
#define BOS_KEY_NUMLOCK 0x97
#define BOS_KEY_CTRL  0x98
#define BOS_KEY_ALT   0x99
#define BOS_KEY_SHIFT 0x9A

// Represents a single keyboard event (press or release)
typedef struct {
    uint8_t keycode;    // Special keycode or ASCII
    char ascii;
    bool pressed;
    bool shift;
    bool ctrl;
    bool alt;
    bool caps_lock;
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
void keyboard_get_event(KeyboardEvent* out_event);
