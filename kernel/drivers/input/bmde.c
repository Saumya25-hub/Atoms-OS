#include "bmde.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"

#ifdef BMDE_DEBUG

BMDE_State bmde_state;

void bmde_init(void) {
    // Clear the state
    uint8_t* ptr = (uint8_t*)&bmde_state;
    for (uint32_t i = 0; i < sizeof(BMDE_State); i++) {
        ptr[i] = 0;
    }

    bmde_state.boot_time_ms = timer_get_ticks();
    bmde_state.cursor_state = "Normal";
    bmde_state.last_hardware_error = "None";
    
    display_print("[BMDE] Diagnostics Engine Initialized\n");
}

#endif
