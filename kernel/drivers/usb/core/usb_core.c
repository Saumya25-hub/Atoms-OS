#include "usb_core.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

#define MAX_USB_DEVICES 128
static USBDevice g_usb_devices[MAX_USB_DEVICES];
static uint32_t g_device_count = 0;

void usb_core_init(void) {
    display_print("[USB CORE] Initializing USB Core Subsystem\n");
    memset(g_usb_devices, 0, sizeof(g_usb_devices));
    g_device_count = 0;
    display_print("[USB CORE] Device model initialized\n");
}

void usb_register_device(USBDevice* dev) {
    if (g_device_count >= MAX_USB_DEVICES) {
        display_print("[USB CORE] Error: Maximum USB devices reached\n");
        return;
    }
    
    // Copy the device structure into the registry
    USBDevice* new_dev = &g_usb_devices[g_device_count++];
    *new_dev = *dev;
    
    display_print("[USB CORE] Registered Device Address: ");
    extern void display_print_dec(uint64_t);
    display_print_dec(new_dev->address);
    display_print(" VID: ");
    extern void display_print_hex(uint64_t);
    display_print_hex(new_dev->vid);
    display_print(" PID: ");
    display_print_hex(new_dev->pid);
    display_print("\n");
}
