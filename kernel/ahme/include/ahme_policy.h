#ifndef ATOMS_AHME_POLICY_H
#define ATOMS_AHME_POLICY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AHME_DRIVER_ACTION_PS2_RESET = 1,
    AHME_DRIVER_ACTION_PS2_COMMAND,
    AHME_DRIVER_ACTION_USB_HID_CLAIM,
    AHME_DRIVER_ACTION_IRQ_REGISTER
} AHMEDriverAction;

typedef enum {
    AHME_POLICY_ALLOW = 0,
    AHME_POLICY_DENY_RISKY,
    AHME_POLICY_REDIRECT_USB_HID
} AHMEPolicyVerdict;

AHMEPolicyVerdict ahme_policy_evaluate(uint32_t driver_id, AHMEDriverAction action, uint32_t param);

#endif // ATOMS_AHME_POLICY_H
