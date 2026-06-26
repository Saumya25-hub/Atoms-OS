#include "mouse.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/interrupt/include/irq.h"
#include "kernel/input/input.h"
#include "kernel/display/display.h"
#include "kernel/timer/include/timer.h"
#include "kernel/input/bmde.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_ERROR 0xFC

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

// Diagnostics
static PS2MouseDiagnostics diag = {0, 0, 0, 0};

void ps2_mouse_get_diagnostics(PS2MouseDiagnostics* out_diag) {
    if (out_diag) {
        *out_diag = diag;
    }
}

// 0: Wait for read, 1: Wait for write
static void ps2_mouse_wait(bool type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 1) == 1) {
                return;
            }
        }
    } else {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 2) == 0) {
                return;
            }
        }
    }
}

// Write to PS/2 Mouse specifically (via 0xD4)
static bool ps2_mouse_write_ack(uint8_t data) {
    int retries = 3;
    while (retries > 0) {
        ps2_mouse_wait(1);
        io_out8(PS2_CMD_PORT, 0xD4);
        ps2_mouse_wait(1);
        io_out8(PS2_DATA_PORT, data);

        ps2_mouse_wait(0);
        uint8_t ack = io_in8(PS2_DATA_PORT);

        if (ack == PS2_ACK) {
            return true;
        } else if (ack == PS2_RESEND) {
            retries--;
        } else {
            // Error or unexpected
            diag.ack_failures++;
            return false;
        }
    }
    diag.ack_failures++;
    return false;
}

static uint8_t ps2_mouse_read(void) {
    ps2_mouse_wait(0);
    return io_in8(PS2_DATA_PORT);
}

static uint64_t mouse_irq_handler(registers_t* regs) {
    (void)regs;
    diag.irq_count++;
    
#ifdef BMDE_DEBUG
    uint64_t t_start = timer_get_ticks();
    bmde_state.irq_count++;
    bmde_state.last_irq_time = t_start;
#endif

    uint8_t status = io_in8(PS2_STATUS_PORT);

    while (status & 0x01) {
        if (!(status & 0x20)) {
            // Not a mouse byte
            io_in8(PS2_DATA_PORT);
            status = io_in8(PS2_STATUS_PORT);
            continue;
        }

        uint8_t byte = io_in8(PS2_DATA_PORT);

        // Synchronization Check
        if (mouse_cycle == 0 && (byte & 0x08) == 0) {
            diag.sync_errors++;
            // Discard out-of-sync byte
            status = io_in8(PS2_STATUS_PORT);
            continue;
        }

        mouse_byte[mouse_cycle] = byte;
        mouse_cycle++;

        if (mouse_cycle == 3) {
            mouse_cycle = 0;
            diag.packet_count++;

            // Decode X and Y
            int32_t dx = mouse_byte[1] - ((mouse_byte[0] << 4) & 0x100);
            int32_t dy = mouse_byte[2] - ((mouse_byte[0] << 3) & 0x100);
            
            // X/Y Overflows
            if (mouse_byte[0] & 0x40) { dx = (dx < 0) ? -255 : 255; }
            if (mouse_byte[0] & 0x80) { dy = (dy < 0) ? -255 : 255; }

            uint8_t buttons = mouse_byte[0] & 0x07; // Left, Right, Middle

#ifdef BMDE_DEBUG
            bmde_state.total_packets++;
            bmde_state.dx = dx;
            bmde_state.dy = dy;

            // Record packet history
            uint32_t h_head = bmde_state.history_head;
            bmde_state.history[h_head].bytes[0] = mouse_byte[0];
            bmde_state.history[h_head].bytes[1] = mouse_byte[1];
            bmde_state.history[h_head].bytes[2] = mouse_byte[2];
            bmde_state.history_head = (h_head + 1) % BMDE_HISTORY_SIZE;
#endif

            kernel_input_push_mouse(dx, dy, buttons);
        }

        status = io_in8(PS2_STATUS_PORT);
    }

#ifdef BMDE_DEBUG
    bmde_state.perf_irq = timer_get_ticks() - t_start;
#endif

    return 0;
}

void ps2_mouse_init(void) {
    uint8_t status;

    // 0. Flush any stale data (crucial if user moved mouse during bootloader)
    while (io_in8(PS2_STATUS_PORT) & 1) {
        io_in8(PS2_DATA_PORT);
    }

    // 1. Enable the auxiliary mouse device
    ps2_mouse_wait(1);
    io_out8(PS2_CMD_PORT, 0xA8);

    // 2. Read Controller Configuration Byte
    ps2_mouse_wait(1);
    io_out8(PS2_CMD_PORT, 0x20);
    ps2_mouse_wait(0);
    status = io_in8(PS2_DATA_PORT);

    // 3. Enable IRQ12 (Bit 1)
    status |= 2;
    
    // 4. Write Controller Configuration Byte
    ps2_mouse_wait(1);
    io_out8(PS2_CMD_PORT, 0x60);
    ps2_mouse_wait(1);
    io_out8(PS2_DATA_PORT, status);

    // 5. Reset/Set Defaults (0xF6)
    if (!ps2_mouse_write_ack(0xF6)) {
        display_print("PS/2 Mouse Init Error: Set Defaults Failed\n");
    }

    // 6. Enable Data Reporting / Streaming (0xF4)
    if (!ps2_mouse_write_ack(0xF4)) {
        display_print("PS/2 Mouse Init Error: Enable Streaming Failed\n");
#ifdef BMDE_DEBUG
        bmde_state.streaming_enabled = false;
        bmde_state.last_hardware_error = "STREAM ENABLE FAILED";
#endif
    } else {
#ifdef BMDE_DEBUG
        bmde_state.streaming_enabled = true;
#endif
    }

    // Register IRQ12 handler
    irq_register_handler(12, mouse_irq_handler);

#ifdef BMDE_DEBUG
    bmde_state.port_ok = true;
    bmde_state.mouse_present = true;
    bmde_state.irq_registered = true;
    bmde_state.irq_enabled = true;
#endif

    display_print("[DIAG] 8042 PS/2 Controller Initialized\n");
    display_print("[DIAG] IRQ12 Handler Registered at IDT Vector 44\n");
    display_print("Professional PS/2 Mouse Stack Initialized.\n");
}
