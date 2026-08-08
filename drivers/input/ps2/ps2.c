#include "drivers/input/ps2/ps2.h"
#include "arch/x86_64/io/port_io.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

static void ps2_init(void) {
    // Basic PS/2 initialization with bounded safety counter
    // Prevents infinite loops on bare-metal hardware with USB legacy keyboard emulation
    int timeout = 1000;
    while ((io_in8(PS2_STATUS_PORT) & 1) && timeout > 0) {
        (void)io_in8(PS2_DATA_PORT);
        timeout--;
    }
}

static uint8_t ps2_read_scancode(void) {
    // In an IRQ handler, the data is guaranteed to be ready.
    // Read directly from the PS/2 Data Port.
    return io_in8(PS2_DATA_PORT);
}

// Global instance of the driver
KeyboardDriver ps2_keyboard_driver = {
    .init = ps2_init,
    .read_scancode = ps2_read_scancode
};
