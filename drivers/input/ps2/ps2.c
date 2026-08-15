#include "drivers/input/ps2/ps2.h"
#include "arch/x86_64/io/port_io.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

static void ps2_wait_write(void) {
    int timeout = 100000;
    while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
        timeout--;
        __asm__ volatile("pause");
    }
}

static void ps2_wait_read(void) {
    int timeout = 100000;
    while (!(io_in8(PS2_STATUS_PORT) & 1) && timeout > 0) {
        timeout--;
        __asm__ volatile("pause");
    }
}

static void ps2_init(void) {
    // 1. Flush output buffer
    int timeout = 1000;
    while ((io_in8(PS2_STATUS_PORT) & 1) && timeout > 0) {
        (void)io_in8(PS2_DATA_PORT);
        timeout--;
    }

    // 2. Read 8042 Controller Configuration Byte
    ps2_wait_write();
    io_out8(PS2_STATUS_PORT, 0x20);
    ps2_wait_read();
    uint8_t config = io_in8(PS2_DATA_PORT);

    // 3. Configure: Enable Port 1 Interrupt (IRQ 1), Enable Scan Translation, Enable Clock
    config |= 0x01;  // IRQ 1 Enable
    config |= 0x40;  // Translation Enable
    config &= ~0x10; // Port 1 Clock Enable

    // 4. Write back Controller Configuration Byte
    ps2_wait_write();
    io_out8(PS2_STATUS_PORT, 0x60);
    ps2_wait_write();
    io_out8(PS2_DATA_PORT, config);

    // 5. Enable Port 1 (Command 0xAE)
    ps2_wait_write();
    io_out8(PS2_STATUS_PORT, 0xAE);

    // 6. Enable Keyboard Scanning (Command 0xF4)
    ps2_wait_write();
    io_out8(PS2_DATA_PORT, 0xF4);

    // 7. Flush ACK / Initial status
    timeout = 1000;
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
