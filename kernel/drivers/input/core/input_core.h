#ifndef KERNEL_INPUT_CORE_H
#define KERNEL_INPUT_CORE_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 2: Universal Event Standardization
// ============================================================================

typedef enum {
    INPUT_EVENT_TYPE_NONE = 0,
    INPUT_EVENT_TYPE_MOTION_RELATIVE, // Raw dx/dy from mice/trackballs
    INPUT_EVENT_TYPE_MOTION_ABSOLUTE, // Clamped x/y from tablets/touchscreens/VMs
    INPUT_EVENT_TYPE_BUTTON,          // Button press/release transitions
    INPUT_EVENT_TYPE_KEY,             // Keyboard key press/release & ASCII
    INPUT_EVENT_TYPE_SCROLL,          // Vertical & horizontal scroll wheels
    INPUT_EVENT_TYPE_TOUCH_CONTACT,   // Multi-touch contact point updates
    INPUT_EVENT_TYPE_DEVICE_STATUS    // Device connect/disconnect/error notifications
} InputEventType;

typedef enum {
    INPUT_DEVICE_TYPE_UNKNOWN = 0,
    INPUT_DEVICE_TYPE_PS2_MOUSE,
    INPUT_DEVICE_TYPE_PS2_KEYBOARD,
    INPUT_DEVICE_TYPE_USB_TABLET,
    INPUT_DEVICE_TYPE_USB_MOUSE,
    INPUT_DEVICE_TYPE_VMMOUSE,
    INPUT_DEVICE_TYPE_TOUCHPAD,
    INPUT_DEVICE_TYPE_PEN
} InputDeviceType;

typedef struct {
    uint32_t device_id;          // Unique numerical ID assigned during device registration
    InputDeviceType device_type; // Hardware classification of the originating device
    InputEventType type;         // Semantic action type
    uint64_t timestamp_us;       // High-resolution timestamp in microseconds
    uint32_t flags;              // Modifier bitmask & hardware overflow/error flags
    
    // Universal data payload supporting all pointing, typing, and tactile devices
    union {
        struct {
            int32_t dx;
            int32_t dy;
            uint32_t buttons;
        } motion_rel;
        
        struct {
            int32_t x;
            int32_t y;
            uint32_t max_x;
            uint32_t max_y;
            uint32_t buttons;
        } motion_abs;
        
        struct {
            uint8_t button_id;   // 0: Left, 1: Right, 2: Middle, 3..N: Pen/Aux/Touch
            bool pressed;
            uint32_t button_mask;// Complete bitmask of all currently held buttons
        } button;
        
        struct {
            uint32_t keycode;
            uint32_t ascii;
            bool pressed;
            uint32_t modifiers;  // Bit 0: Shift, Bit 1: Ctrl, Bit 2: Alt, Bit 3: CapsLock
        } key;
        
        struct {
            int32_t delta_x;
            int32_t delta_y;
        } scroll;
        
        struct {
            uint32_t contact_id; // Unique touch finger ID for multi-touch tracking
            int32_t x;
            int32_t y;
            uint32_t pressure;
            bool active;
        } touch;
    } data;
} InputCoreEvent;

// ============================================================================
// Modular Dispatcher & Consumer Registration
// ============================================================================

typedef enum {
    INPUT_PRIORITY_POINTER_ENGINE = 0, // Tier 0: Real-time cursor/pointer tracking (Phase 3/5)
    INPUT_PRIORITY_DESKTOP_SHELL  = 1, // Tier 1: Window Manager & Z-order hit testing
    INPUT_PRIORITY_WIDGET_CONTROL = 2, // Tier 2: UI controls, buttons, text boxes
    INPUT_PRIORITY_APPLICATION    = 3  // Tier 3: Userspace application delivery
} InputConsumerPriority;

// Return true if event was consumed/handled, false to allow propagation to lower tiers
typedef bool (*InputConsumerCallback)(const InputCoreEvent* event, void* user_data);

// ============================================================================
// Input Core API
// ============================================================================

// Initialize the central Input Core subsystem
void input_core_init(void);

// Push a standardized event into the central lockless queue
bool input_core_push_event(const InputCoreEvent* event);

// Pop an event from the central queue (non-blocking)
bool input_core_pop_event(InputCoreEvent* out_event);

// Register an event consumer callback at a specific priority tier
bool input_core_register_consumer(const char* name, InputConsumerPriority priority, InputConsumerCallback callback, void* user_data);

// Dispatch all pending events in the queue to registered consumers by priority
void input_core_dispatch_events(void);

// Retrieve diagnostic telemetry counters
void input_core_get_diagnostics(uint32_t* total_pushed, uint32_t* total_dispatched, uint32_t* total_dropped, uint32_t* queue_size);

#endif // KERNEL_INPUT_CORE_H
