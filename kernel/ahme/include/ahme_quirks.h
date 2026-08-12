#ifndef ATOMS_AHME_QUIRKS_H
#define ATOMS_AHME_QUIRKS_H

#include <stdint.h>
#include <stdbool.h>
#include "ahme_profile.h"

#define AHME_QUIRK_INTEL_H81_PS2_SMM_UNRELIABLE (1 << 0)
#define AHME_QUIRK_PREFER_USB_HID_INPUT         (1 << 1)
#define AHME_QUIRK_NO_DESTRUCTIVE_8042_RESET    (2 << 0)
#define AHME_QUIRK_STRICT_IO_WAIT_TIMEOUT       (3 << 0)

uint32_t ahme_quirks_evaluate(const AHMEHardwareProfile *profile);
bool ahme_quirks_has(uint32_t quirk_bit);

#endif // ATOMS_AHME_QUIRKS_H
