# BOS Pawn Driver Engine (BPDE) Architecture

## 1. Overview
The BOS Pawn Driver Engine (BPDE) is the new standard, production-grade userspace input runtime for ATOMS OS. It serves as the front-line input bridge between the ATOMS kernel (BWE) and all userspace applications, replacing legacy DOOM-specific input patches with a stable, reusable event pipeline.

## 2. Event Model & ABI
BPDE introduces a highly stable Event ABI that preserves physical, logical, and character state independently, ensuring no information is destroyed during routing.

```c
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
    bos_input_type_t type;
    uint32_t window_id;     // Target window
    uint64_t timestamp;
    union {
        struct {
            uint32_t scancode; // Physical hardware scancode
            uint32_t key;      // Logical BOS Key Identity (BOS_KEY_ESCAPE, etc.)
            uint32_t character;// ASCII/Unicode mapping
            uint32_t modifiers;// Shift, Ctrl, Alt
            bool repeat;       // Is this an auto-repeat event?
        } key;
        struct {
            int32_t screen_x;  // Absolute screen X
            int32_t screen_y;  // Absolute screen Y
            int32_t local_x;   // Window-local X
            int32_t local_y;   // Window-local Y
            int32_t delta_x;   // Relative movement X
            int32_t delta_y;   // Relative movement Y
            uint8_t buttons;   // Mouse button state
            uint8_t changed_button; // The button that triggered the up/down event
        } mouse;
    } data;
} bos_input_event_t;
```

## 3. Queue Policy
To solve the queue-flooding issue caused by high-frequency mouse movement:
1. **Per-Process Queues:** The kernel maintains a dedicated input queue for each process (or BWE maintains it per window/PID).
2. **Coalescing:** If a `BOS_INPUT_MOUSE_MOVE` event is pushed and the most recent event in the queue is *also* a `BOS_INPUT_MOUSE_MOVE`, the kernel updates the existing event with the newest position and accumulates the delta. It does not consume a new queue slot.
3. **Semantic Integrity:** Key presses, mouse clicks, and focus changes are *never* coalesced or dropped. 

## 4. Focus Ownership & Routing
- **Mouse Routing:** BWE continues to perform Hit-Testing. Mouse events are routed to the `owner_pid` of the leaf window.
- **Keyboard Routing:** A global `g_focused_window_id` tracks keyboard focus. When a mouse click (Mouse Down) occurs on a window, focus is transferred. 
- **Focus Events:** BWE dispatches `BOS_INPUT_FOCUS_LOST` to the previous owner and `BOS_INPUT_FOCUS_GAINED` to the new owner. Keyboard events are routed strictly to the process owning the focused window.

## 5. Coordinate Model
1. **Hardware (VMMouse/PS2):** Supplies Deltas or Absolute Screen Bounds.
2. **Kernel Input Core:** Maintains absolute `screen_x` and `screen_y`.
3. **BWE Hit-Test:** Identifies the target window using absolute bounds.
4. **BPDE (Kernel -> Userspace):** Subtracts `window.screen_bounds.x/y` to produce `local_x/local_y` before injecting the event into the process queue. The application receives BOTH absolute and local coordinates.

## 6. Userspace API (libbos)
BPDE provides the following state and event API for C applications:
```c
// Event-driven API
int BOS_InputPollEvent(bos_input_event_t* out_event);

// State-driven API (Useful for Games like DOOM)
bool BOS_InputGetKeyState(uint32_t bos_key);
void BOS_InputGetMouseState(int32_t* x, int32_t* y, uint8_t* buttons);
```

## 7. DOOM Compatibility Boundary
DOOM will link against BPDE. The kernel will NO LONGER translate keys specifically for DOOM.
In `doomgeneric_signaturesos.c`:
- `DG_GetKey` will call `BOS_InputPollEvent(&ev)`.
- It will translate `ev.data.key.key` (BOS Identity) to `KEY_ESCAPE`, `KEY_ENTER`, etc., entirely within userspace.

## 8. Latency Strategy
- Event generation occurs directly inside `BWE_PumpEvents` or asynchronous dispatch.
- Coalescing reduces the number of syscalls required by userspace.
- `BOS_InputPollEvent` is a single non-blocking syscall that copies a pending event.
- State-tracking arrays (`bos_key_state[256]`) are updated seamlessly within the userspace `BOS_InputPollEvent` wrapper, minimizing kernel transitions for state queries.
