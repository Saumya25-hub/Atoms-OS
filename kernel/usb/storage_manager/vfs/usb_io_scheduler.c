#include "../include/usb_disk.h"
#include "kernel/drivers/display/display.h"

void usb_io_scheduler_init(void) {
    display_print("[USM SCHEDULER] Elevator I/O Scheduler Initialized.\n");
}
