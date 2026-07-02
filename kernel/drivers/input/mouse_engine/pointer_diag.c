#include "pointer_diag.h"

static PointerDiagnostics diag_state;

void pointer_diag_init(void) {
    diag_state.raw_dx = 0;
    diag_state.raw_dy = 0;
    diag_state.filtered_dx = 0;
    diag_state.filtered_dy = 0;
    diag_state.abs_x = 0;
    diag_state.abs_y = 0;
    diag_state.prev_x = 0;
    diag_state.prev_y = 0;
    diag_state.packet_count = 0;
    diag_state.dropped_packets = 0;
    diag_state.overflow_packets = 0;
    diag_state.clamp_count = 0;
    diag_state.movement_rate = 0;
    diag_state.sync_good = true;
}

void pointer_diag_update_raw(int32_t dx, int32_t dy) {
    diag_state.raw_dx = dx;
    diag_state.raw_dy = dy;
}

void pointer_diag_update_filtered(int32_t dx, int32_t dy) {
    diag_state.filtered_dx = dx;
    diag_state.filtered_dy = dy;
}

void pointer_diag_update_abs(int32_t x, int32_t y, int32_t px, int32_t py) {
    diag_state.abs_x = x;
    diag_state.abs_y = y;
    diag_state.prev_x = px;
    diag_state.prev_y = py;
}

void pointer_diag_inc_packet(void) { diag_state.packet_count++; }
void pointer_diag_inc_drop(void) { diag_state.dropped_packets++; }
void pointer_diag_inc_overflow(void) { diag_state.overflow_packets++; }
void pointer_diag_inc_clamp(void) { diag_state.clamp_count++; }
void pointer_diag_set_sync(bool good) { diag_state.sync_good = good; }
const PointerDiagnostics* pointer_diag_get(void) { return &diag_state; }
