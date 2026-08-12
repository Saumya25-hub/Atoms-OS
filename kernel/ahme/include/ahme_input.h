#ifndef ATOMS_AHME_INPUT_H
#define ATOMS_AHME_INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AHME_INPUT_ROUTE_PS2_ACTIVE = 0,
    AHME_INPUT_ROUTE_USB_HID_ACTIVE,
    AHME_INPUT_ROUTE_FALLBACK_HYBRID
} AHMEInputRouteMode;

void ahme_input_init(void);
AHMEInputRouteMode ahme_input_get_route(void);
void ahme_input_notify_irq12_activity(void);
void ahme_input_notify_usb_hid_activity(void);

#endif // ATOMS_AHME_INPUT_H
