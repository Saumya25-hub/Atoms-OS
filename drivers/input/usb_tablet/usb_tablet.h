#ifndef DRIVERS_INPUT_USB_TABLET_H
#define DRIVERS_INPUT_USB_TABLET_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the USB Tablet input driver skeleton
void usb_tablet_init(void);

// Report an absolute coordinate event from USB Tablet controller or hypervisor integration
// raw_x and raw_y are absolute coordinates in normalized (0-32767) or direct screen pixels
void usb_tablet_report_event(uint32_t raw_x, uint32_t raw_y, uint8_t buttons);

#endif // DRIVERS_INPUT_USB_TABLET_H
