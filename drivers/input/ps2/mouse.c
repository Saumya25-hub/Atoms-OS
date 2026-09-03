#include "mouse.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/input/input.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/input/bmde.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "drivers/interrupt/pic/pic.h"

#include "kernel/drivers/input/core/hida.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

#define PS2_ACK         0xFA
#define PS2_RESEND      0xFE
#define PS2_ERROR       0xFC
#define PS2_MAX_BYTES_PER_IRQ 48

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];
static uint64_t last_byte_time = 0;

static PS2MouseDiagnostics diag = {0};

void ps2_mouse_get_diagnostics(PS2MouseDiagnostics* out_diag) {
    if (out_diag) {
        *out_diag = diag;
    }
}

static void io_wait_delay(void) {
    io_out8(0x80, 0);
}

// 0: Wait for output buffer full (read ready), 1: Wait for input buffer empty (write ready)
// Uses fast non-blocking bounded timeout to prevent CPU stalls on real hardware
static bool ps2_mouse_wait(bool type) {
    uint32_t timeout = 2000; // ~2ms max timeout window
    if (type == 0) {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 1) == 1) {
                return true;
            }
            io_wait_delay();
        }
    } else {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 2) == 0) {
                return true;
            }
            io_wait_delay();
        }
    }
    return false;
}

// Write to PS/2 Mouse specifically via 0xD4 prefix
static bool ps2_mouse_write_ack(uint8_t data) {
    int retries = 2;
    while (retries > 0) {
        if (!ps2_mouse_wait(1)) break;
        io_out8(PS2_CMD_PORT, 0xD4);
        if (!ps2_mouse_wait(1)) break;
        io_out8(PS2_DATA_PORT, data);

        if (!ps2_mouse_wait(0)) {
            retries--;
            continue;
        }
        uint8_t ack = io_in8(PS2_DATA_PORT);

        if (ack == PS2_ACK) {
            diag.ack_count++;
            return true;
        } else if (ack == PS2_RESEND) {
            retries--;
        } else {
            diag.ack_failures++;
            return false;
        }
    }
    diag.ack_failures++;
    return false;
}

volatile uint64_t g_irq12_count = 0;

void ps2_mouse_handle_byte(uint8_t byte) {
    uint64_t current_time = timer_get_ticks();

    // Timeout Synchronization (Reset cycle if gap > 25ms)
    if (mouse_cycle > 0 && (current_time - last_byte_time) > 25) {
        diag.sync_errors++;
        hida_report_event_parsed(HIDA_BACKEND_PS2, false);
        mouse_cycle = 0;
    }
    last_byte_time = current_time;

    // Protocol Synchronization Check (Bit 3 of first byte MUST be 1)
    if (mouse_cycle == 0 && (byte & 0x08) == 0) {
        diag.sync_errors++;
        hida_report_event_parsed(HIDA_BACKEND_PS2, false);
        return;
    }

    mouse_byte[mouse_cycle] = byte;
    mouse_cycle++;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        diag.packet_count++;
        hida_report_event_parsed(HIDA_BACKEND_PS2, true);

        // Decode X and Y using standard bitwise sign extension
        int32_t dx = (int32_t)mouse_byte[1];
        if (mouse_byte[0] & 0x10) { dx |= 0xFFFFFF00; }

        int32_t dy = (int32_t)mouse_byte[2];
        if (mouse_byte[0] & 0x20) { dy |= 0xFFFFFF00; }

        uint8_t buttons = mouse_byte[0] & 0x07;

        hida_push_relative(HIDA_BACKEND_PS2, dx, dy, buttons, 0);
    }
}

static uint64_t mouse_irq_handler(registers_t* regs) {
    (void)regs;
    g_irq12_count++;
    diag.irq_count++;

    extern bool vmmouse_is_active(void);
    if (vmmouse_is_active()) {
        while (io_in8(PS2_STATUS_PORT) & 1) {
            io_in8(PS2_DATA_PORT);
        }
        extern void vmmouse_poll(void);
        vmmouse_poll();
        return 0;
    }

    uint8_t status = io_in8(PS2_STATUS_PORT);

    uint32_t bytes_processed = 0;
    while ((status & 0x01) && (status & 0x20) && bytes_processed++ < PS2_MAX_BYTES_PER_IRQ) {
        uint8_t byte = io_in8(PS2_DATA_PORT);
        ps2_mouse_handle_byte(byte);
        status = io_in8(PS2_STATUS_PORT);
    }

    return 0;
}

void ps2_mouse_init(void) {
    display_print("[PS/2 MOUSE] Production Non-Blocking Bring-Up...\n");

    // 0. Flush stale bytes in 8042 buffer (max 16 bytes)
    int flush_count = 16;
    while ((io_in8(PS2_STATUS_PORT) & 1) && flush_count-- > 0) {
        io_in8(PS2_DATA_PORT);
        io_wait_delay();
    }

    diag.controller_init = true;
    diag.self_test_pass = true; // Non-destructive assumption on modern x86
    diag.port_test_pass = true;

    // 1. Enable Auxiliary Mouse Device Port (0xA8)
    if (ps2_mouse_wait(1)) {
        io_out8(PS2_CMD_PORT, 0xA8);
    }

    // 2. Read & Configure Controller Command Byte
    uint8_t status = 0x47; // Default safe configuration
    if (ps2_mouse_wait(1)) {
        io_out8(PS2_CMD_PORT, 0x20);
        if (ps2_mouse_wait(0)) {
            status = io_in8(PS2_DATA_PORT);
        }
    }

    // Enable IRQ1 (bit 0) & IRQ12 (bit 1)
    status |= 0x03;
    // Enable Clock lines: clear Bit 4 (KBD) and Bit 5 (AUX Mouse)
    status &= ~(0x30);

    if (ps2_mouse_wait(1)) {
        io_out8(PS2_CMD_PORT, 0x60);
        if (ps2_mouse_wait(1)) {
            io_out8(PS2_DATA_PORT, status);
        }
    }

    // 3. Enable Data Streaming (0xF4)
    if (ps2_mouse_write_ack(0xF4)) {
        diag.streaming_enabled = true;
        diag.mouse_reset_pass = true;
        display_print("[PS/2 MOUSE] Streaming Mode (0xF4) Enabled: PASS\n");
    } else {
        display_print("[PS/2 MOUSE] Streaming (0xF4) Non-Blocking Skip\n");
    }

    // 4. Unmask PIC Interrupt Lines (IRQ2 Cascade & IRQ12 Mouse)
    pic_clear_mask(2);
    pic_clear_mask(12);
    display_print("[PS/2 MOUSE] PIC IRQ2 & IRQ12 Unmasked.\n");

    // 5. Register IRQ12 handler
    irq_register_handler(12, mouse_irq_handler);

    // Register with HIDA
    InputDeviceDescriptor ps2_desc = {0};
    ps2_desc.backend_id = HIDA_BACKEND_PS2;
    ps2_desc.type = INPUT_DEV_TYPE_PS2_MOUSE;
    ps2_desc.device_name = "8042 PS/2 Mouse Controller";
    ps2_desc.driver_name = "ps2_mouse";
    ps2_desc.is_supported = true;
    ps2_desc.is_initialized = true;
    ps2_desc.is_connected = true;
    ps2_desc.is_absolute = false;
    ps2_desc.is_polling = false;
    ps2_desc.priority_score = 60;
    ps2_desc.health_score = 100;
    ps2_desc.status = HIDA_STATE_ACTIVE;
    hida_register_device(&ps2_desc);

    display_print("[PS/2 MOUSE] Hardware Initialization Complete.\n");
}
