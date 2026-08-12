#include "kernel/ahme/include/ahme_quirks.h"

static uint32_t s_active_quirks = 0;

uint32_t ahme_quirks_evaluate(const AHMEHardwareProfile *profile) {
    s_active_quirks = 0;

    if (!profile) return 0;

    // Intel H81 Haswell Chipset Quirks
    if (profile->chipset_family == AHME_CHIPSET_INTEL_H81_HASWELL) {
        s_active_quirks |= AHME_QUIRK_INTEL_H81_PS2_SMM_UNRELIABLE;
        s_active_quirks |= AHME_QUIRK_PREFER_USB_HID_INPUT;
        s_active_quirks |= AHME_QUIRK_NO_DESTRUCTIVE_8042_RESET;
        s_active_quirks |= AHME_QUIRK_STRICT_IO_WAIT_TIMEOUT;
    }

    return s_active_quirks;
}

bool ahme_quirks_has(uint32_t quirk_bit) {
    return (s_active_quirks & quirk_bit) != 0;
}
