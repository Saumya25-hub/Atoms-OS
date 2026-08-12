#include "kernel/ahme/include/ahme_policy.h"
#include "kernel/ahme/include/ahme_quirks.h"

AHMEPolicyVerdict ahme_policy_evaluate(uint32_t driver_id, AHMEDriverAction action, uint32_t param) {
    (void)driver_id;
    (void)param;

    if (action == AHME_DRIVER_ACTION_PS2_RESET) {
        if (ahme_quirks_has(AHME_QUIRK_NO_DESTRUCTIVE_8042_RESET)) {
            return AHME_POLICY_DENY_RISKY;
        }
    }

    if (action == AHME_DRIVER_ACTION_PS2_COMMAND) {
        if (ahme_quirks_has(AHME_QUIRK_PREFER_USB_HID_INPUT)) {
            return AHME_POLICY_REDIRECT_USB_HID;
        }
    }

    return AHME_POLICY_ALLOW;
}
