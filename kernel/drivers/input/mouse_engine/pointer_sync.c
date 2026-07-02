#include "pointer_sync.h"
#include "pointer_diag.h"

void pointer_sync_init(void) {
    pointer_diag_set_sync(true);
}

bool pointer_sync_validate_packet(int32_t dx, int32_t dy, bool overflow_x, bool overflow_y) {
    (void)dx;
    (void)dy;
    
    if (overflow_x || overflow_y) {
        pointer_diag_inc_overflow();
        pointer_diag_set_sync(false);
        return false; // Drop packet
    }
    
    pointer_diag_set_sync(true);
    return true; // Valid packet
}
