#include "mouse_engine.h"
#include "pointer_diag.h"
#include "pointer_bounds.h"
#include "pointer_filter.h"
#include "pointer_sync.h"
#include "pointer_manager.h"
#include "../bmde.h"

void mouse_engine_init(uint32_t screen_width, uint32_t screen_height) {
    pointer_diag_init();
    pointer_bounds_init(screen_width, screen_height);
    pointer_filter_init();
    pointer_sync_init();
    pointer_manager_init(screen_width / 2, screen_height / 2);
}

void mouse_engine_update_resolution(uint32_t screen_width, uint32_t screen_height) {
    pointer_bounds_update(screen_width, screen_height);
}

void mouse_engine_push_packet(int32_t raw_dx, int32_t raw_dy, uint8_t buttons, bool overflow_x, bool overflow_y) {
    pointer_diag_inc_packet();
    pointer_diag_update_raw(raw_dx, raw_dy);

    if (!pointer_sync_validate_packet(raw_dx, raw_dy, overflow_x, overflow_y)) {
        pointer_diag_inc_drop();
        return; // Dropped
    }

    int32_t filtered_dx = 0;
    int32_t filtered_dy = 0;
    pointer_filter_apply(raw_dx, raw_dy, &filtered_dx, &filtered_dy);

#ifdef BMDE_DEBUG
    // Write to the last packet (which mouse.c just pushed)
    uint32_t h_head = (bmde_state.history_head - 1 + BMDE_HISTORY_SIZE) % BMDE_HISTORY_SIZE;
    bmde_state.history[h_head].filtered_dx = filtered_dx;
    bmde_state.history[h_head].filtered_dy = filtered_dy;
#endif

    pointer_manager_apply_movement(filtered_dx, filtered_dy, buttons);
}
