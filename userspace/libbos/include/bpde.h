#ifndef BPDE_H
#define BPDE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BOS_INPUT_NONE = 0,
    BOS_INPUT_KEY_DOWN,
    BOS_INPUT_KEY_UP,
    BOS_INPUT_MOUSE_MOVE,
    BOS_INPUT_MOUSE_DOWN,
    BOS_INPUT_MOUSE_UP,
    BOS_INPUT_FOCUS_GAINED,
    BOS_INPUT_FOCUS_LOST
} bos_input_type_t;

typedef struct {
    uint32_t type;          // bos_input_type_t
    uint32_t window_id;     // Target window
    uint64_t timestamp;
    union {
        struct {
            uint32_t scancode; // Physical hardware scancode
            uint32_t key;      // Logical BOS Key Identity (BOS_KEY_ESCAPE, etc.)
            uint32_t character;// ASCII/Unicode mapping
            uint32_t modifiers;// Shift, Ctrl, Alt
            uint32_t repeat;   // Is this an auto-repeat event?
        } key;
        struct {
            int32_t screen_x;  // Absolute screen X
            int32_t screen_y;  // Absolute screen Y
            int32_t local_x;   // Window-local X
            int32_t local_y;   // Window-local Y
            int32_t delta_x;   // Relative movement X
            int32_t delta_y;   // Relative movement Y
            uint32_t buttons;  // Mouse button state
            uint32_t changed_button; // The button that triggered the up/down event
        } mouse;
    } data;
} bos_input_event_t;

// Event-driven API
int BOS_InputPollEvent(bos_input_event_t* out_event);

// State-driven API
bool BOS_InputGetKeyState(uint32_t bos_key);
void BOS_InputGetMouseState(int32_t* x, int32_t* y, uint32_t* buttons);

#endif // BPDE_H
