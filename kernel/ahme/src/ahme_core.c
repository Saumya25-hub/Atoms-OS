#include "kernel/ahme/include/ahme.h"
#include "kernel/drivers/display/display.h"
#include "kernel/debug/abde/abde.h"

extern void com1_puts(const char *s);

static AHMEState s_ahme_state = {0};

void ahme_init(void) {
    com1_puts("[AHME] Initializing ATOMS Hardware Management Engine (HAL Chief Officer)...\r\n");

    // 1. Discover Hardware Capability Profile
    ahme_profile_discover(&s_ahme_state.profile);

    // 2. Evaluate Hardware Quirk Database
    s_ahme_state.active_quirks_mask = ahme_quirks_evaluate(&s_ahme_state.profile);

    // 3. Register Core Drivers with Health Supervisor
    ahme_health_register_driver(1, "ps2_mouse");
    ahme_health_register_driver(2, "ps2_keyboard");
    ahme_health_register_driver(3, "usb_hid");
    ahme_health_register_driver(4, "vbe_gop");

    // 4. Initialize Input Router
    ahme_input_init();

    s_ahme_state.is_initialized = true;

    com1_puts("[AHME] Hardware Capability Profile Generated:\r\n");
    com1_puts("       Motherboard: "); com1_puts(s_ahme_state.profile.motherboard_name); com1_puts("\r\n");
    if (s_ahme_state.profile.chipset_family == AHME_CHIPSET_INTEL_H81_HASWELL) {
        com1_puts("[AHME] Intel H81 Chipset Detected. Applying Safety Policy (Prefer USB HID / Non-Blocking PS/2 Probe).\r\n");
    }
}

AHMEState* ahme_get_state(void) {
    return &s_ahme_state;
}

void ahme_log_telemetry(void) {
    s_ahme_state.uptime_ticks++;
}
