#ifndef KERNEL_POINTER_STATE_H
#define KERNEL_POINTER_STATE_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Authoritative Pointer State Singleton
// ============================================================================
// The single source of truth for all pointer coordinates, kinematics, and button states.
// ============================================================================

typedef struct {
    // 1. Authoritative Coordinates (Integer screen pixels)
    int32_t current_x;
    int32_t current_y;
    int32_t previous_x;
    int32_t previous_y;
    
    // 2. Raw & Sub-Pixel Tracking (16.16 Fixed-Point representation)
    int32_t raw_x;
    int32_t raw_y;
    int32_t subpixel_x;         // Accumulated fractional X (16.16 fixed-point)
    int32_t subpixel_y;         // Accumulated fractional Y (16.16 fixed-point)
    
    // 3. Motion Kinematics
    int32_t delta_x;            // Frame movement delta X (integer pixels)
    int32_t delta_y;            // Frame movement delta Y (integer pixels)
    int32_t velocity_raw;       // Instantaneous speed (pixels per second * 100)
    int32_t acceleration_factor;// Applied ballistic multiplier (16.16 fixed-point)
    
    // 4. Button & Tactile State
    uint32_t button_mask;       // Complete bitmask of currently held buttons
    uint32_t button_just_pressed;  // Buttons pressed during this exact event cycle
    uint32_t button_just_released; // Buttons released during this exact event cycle
    uint8_t  click_count;       // 1 = Single, 2 = Double, 3 = Triple click
    
    // 5. Interaction & Drag State Machine
    bool is_dragging;           // True if movement occurred while button 0 is held
    int32_t drag_start_x;       // Origin X where drag initiated
    int32_t drag_start_y;       // Origin Y where drag initiated
    uint32_t drag_button;       // Button ID driving the current drag
    
    // 6. Focus & Capture Metadata
    uint32_t capture_window_id; // Window ID holding exclusive mouse capture (0 = none)
    uint32_t hover_window_id;   // Window ID currently beneath the cursor
    uint64_t timestamp_us;      // High-resolution timestamp of last update
    uint32_t device_source_id;  // Originating hardware device ID from Input Core
    uint32_t pointer_id;        // Logical pointer ID (for future multi-pointer/multi-touch)
} PointerState;

// Initialize the authoritative PointerState singleton
void pointer_state_init(uint32_t initial_x, uint32_t initial_y);

// Retrieve read-only access to the central PointerState singleton
const PointerState* pointer_state_get(void);

// Retrieve mutable access to the central PointerState singleton (Internal Pointer Engine use only)
PointerState* pointer_state_get_mutable(void);

// Update spatial kinematics and timestamps
void pointer_state_update_position(int32_t x, int32_t y, int32_t raw_x, int32_t raw_y,
                                   int32_t sub_x, int32_t sub_y, int32_t dx, int32_t dy,
                                   int32_t vel, int32_t accel, uint64_t ts, uint32_t dev_id);

// Update button transitions, click counts, and drag state
void pointer_state_update_buttons(uint32_t mask, uint32_t pressed, uint32_t released,
                                  uint8_t clicks, bool dragging, int32_t drag_x,
                                  int32_t drag_y, uint32_t drag_btn);

#endif // KERNEL_POINTER_STATE_H
