#ifndef KERNEL_POINTER_BUTTONS_H
#define KERNEL_POINTER_BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Button Processing & Drag State Machine
// ============================================================================

#define POINTER_DRAG_THRESHOLD_PX 4
#define POINTER_DOUBLE_CLICK_TIME_US 300000ULL // 300 ms

// Initialize button processing module
void pointer_buttons_init(void);

// Process button state transitions, drag threshold evaluation, and click counts
void pointer_buttons_process(uint32_t new_button_mask, int32_t current_x, int32_t current_y,
                             uint64_t timestamp_us, uint32_t* out_pressed, uint32_t* out_released,
                             uint8_t* out_click_count, bool* out_is_dragging,
                             int32_t* out_drag_start_x, int32_t* out_drag_start_y,
                             uint32_t* out_drag_button);

#endif // KERNEL_POINTER_BUTTONS_H
