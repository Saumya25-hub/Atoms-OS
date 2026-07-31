#ifndef BWE_PROCESS_QUEUE_H
#define BWE_PROCESS_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_INPUT_MAX_QUEUE_SIZE 512

typedef enum {
    BOS_INPUT_NONE = 0,
    BOS_INPUT_KEY_DOWN,
    BOS_INPUT_KEY_UP,
    BOS_INPUT_MOUSE_MOVE,
    BOS_INPUT_MOUSE_DOWN,
    BOS_INPUT_MOUSE_UP,
    BOS_INPUT_FOCUS_GAINED,
    BOS_INPUT_FOCUS_LOST
} BOS_InputType;

typedef struct {
    uint32_t type;          // BOS_InputType
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
} BOS_InputEvent;

// Initializes the process queue subsystem
void bwe_process_queue_init(void);

// Pushes an event to the target process ID queue. 
// Implements mouse coalescing to prevent queue overflow.
void bwe_process_queue_push(uint32_t owner_pid, const BOS_InputEvent* event);

// Pops the oldest event for a given process. Returns true if an event was popped.
bool bwe_process_queue_pop(uint32_t owner_pid, BOS_InputEvent* out_event);

#endif // BWE_PROCESS_QUEUE_H
