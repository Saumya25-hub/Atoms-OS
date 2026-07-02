#include "pointer_manager.h"
#include "pointer_bounds.h"
#include "pointer_diag.h"
#include "../bmde.h"

// Forward declaration for the compatibility layer in input.c
extern void kernel_input_push_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons);

static int32_t abs_x = 0;
static int32_t abs_y = 0;
static int32_t prev_x = 0;
static int32_t prev_y = 0;

void pointer_manager_init(uint32_t initial_x, uint32_t initial_y) {
    abs_x = initial_x;
    abs_y = initial_y;
    prev_x = initial_x;
    prev_y = initial_y;
}

void pointer_manager_apply_movement(int32_t dx, int32_t dy, uint8_t buttons) {
    prev_x = abs_x;
    prev_y = abs_y;

    abs_x += dx;
    abs_y -= dy; // PS/2 y is bottom-up

#ifdef BMDE_DEBUG
    uint32_t h_head = (bmde_state.history_head - 1 + BMDE_HISTORY_SIZE) % BMDE_HISTORY_SIZE;
    bmde_state.history[h_head].pre_clamp_x = abs_x;
    bmde_state.history[h_head].pre_clamp_y = abs_y;
#endif

    pointer_bounds_clamp(&abs_x, &abs_y);
    pointer_diag_update_abs(abs_x, abs_y, prev_x, prev_y);

#ifdef BMDE_DEBUG
    bmde_state.history[h_head].post_clamp_x = abs_x;
    bmde_state.history[h_head].post_clamp_y = abs_y;
    bmde_state.history[h_head].clamped = (bmde_state.history[h_head].pre_clamp_x != abs_x) || (bmde_state.history[h_head].pre_clamp_y != abs_y);
#endif

    // Push absolute state back to the legacy OS layer
    kernel_input_push_mouse_absolute(abs_x, abs_y, buttons);
}
