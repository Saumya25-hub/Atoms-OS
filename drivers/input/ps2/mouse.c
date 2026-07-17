#include "mouse.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/input/input.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/input/bmde.h"
#include "kernel/drivers/input/input_abstraction.h"
#include "drivers/input/vmmouse/vmmouse.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_ERROR 0xFC
#define VMMOUSE_MAX_PACKETS_PER_IRQ 4
#define PS2_MAX_BYTES_PER_IRQ 48

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];
static uint64_t last_byte_time = 0;

// Diagnostics
static PS2MouseDiagnostics diag = {0, 0, 0, 0};

void ps2_mouse_get_diagnostics(PS2MouseDiagnostics* out_diag) {
    if (out_diag) {
        *out_diag = diag;
    }
}

// 0: Wait for read, 1: Wait for write
static void ps2_mouse_wait(bool type) {
    uint32_t timeout = 50000;
    if (type == 0) {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 1) == 1) {
                return;
            }
            __asm__ volatile("pause");
        }
    } else {
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 2) == 0) {
                return;
            }
            __asm__ volatile("pause");
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

volatile uint64_t g_irq12_count = 0;

static uint64_t mouse_irq_handler(registers_t* regs) {
    (void)regs;
    g_irq12_count++;
    diag.irq_count++;
    
#ifdef BMDE_DEBUG
    uint64_t t_start = timer_get_ticks();
    bmde_state.irq_count++;
    bmde_state.last_irq_time = t_start;
#endif

    uint8_t status = io_in8(PS2_STATUS_PORT);

    // VMMouse Integration: DRAIN IMMEDIATELY upon any IRQ12.
    // QEMU/VirtualBox may not send valid 3-byte dummy packets.  The hardware
    // path is deliberately bounded: motion is coalesced below, while button
    // transitions remain individual events.
    if (vmmouse_is_active()) {
        int32_t vm_x, vm_y;
        uint8_t vm_buttons;
        for (uint32_t packet = 0; packet < VMMOUSE_MAX_PACKETS_PER_IRQ; packet++) {
            if (!vmmouse_read(&vm_x, &vm_y, &vm_buttons)) {
                break;
            }
            input_push_absolute(vm_x, vm_y, vm_buttons, 0);
        }
    }

    uint32_t bytes_processed = 0;
    while ((status & 0x01) && bytes_processed++ < PS2_MAX_BYTES_PER_IRQ) {
        if (!(status & 0x20)) {
            // Keyboard byte. Do not read it in the mouse handler.
            break;
        }

        uint8_t byte = io_in8(PS2_DATA_PORT);
        uint64_t current_time = timer_get_ticks();

        // Timeout Synchronization (Reset cycle if gap > 25ms to prevent VM jitter desync)
        if (mouse_cycle > 0 && (current_time - last_byte_time) > 25) {
            diag.sync_errors++;
            mouse_cycle = 0;
        }
        last_byte_time = current_time;

        // Protocol Synchronization Check
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

            // Decode X and Y using standard bitwise sign extension
            int32_t dx = (int32_t)mouse_byte[1];
            if (mouse_byte[0] & 0x10) { dx |= 0xFFFFFF00; } // Sign extend negative

            int32_t dy = (int32_t)mouse_byte[2];
            if (mouse_byte[0] & 0x20) { dy |= 0xFFFFFF00; } // Sign extend negative

            // Do NOT scale dx/dy. VirtualBox Mouse Integration relies on exact 1:1 tracking
            // to keep the host and guest cursor in sync. Scaling causes massive desync and corner shooting.
            
            uint8_t buttons = mouse_byte[0] & 0x07; // Left, Right, Middle
            bool overflow_x = (mouse_byte[0] & 0x40) != 0;
            bool overflow_y = (mouse_byte[0] & 0x80) != 0;

#ifdef BMDE_DEBUG
            bmde_state.total_packets++;
            bmde_state.dx = dx;
            bmde_state.dy = dy;

            // Record packet history
            uint32_t h_head = bmde_state.history_head;
            bmde_state.history[h_head].bytes[0] = mouse_byte[0];
            bmde_state.history[h_head].bytes[1] = mouse_byte[1];
            bmde_state.history[h_head].bytes[2] = mouse_byte[2];
            bmde_state.history[h_head].raw_dx = dx;
            bmde_state.history[h_head].raw_dy = dy;
            bmde_state.history[h_head].overflow_x = overflow_x;
            bmde_state.history[h_head].overflow_y = overflow_y;
            bmde_state.history_head = (h_head + 1) % BMDE_HISTORY_SIZE;
#endif

            if (!vmmouse_is_active()) {
                input_push_relative(dx, dy, buttons, 0);
            }
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

    // 5. Send Reset (0xFF) to restore standard 3-byte relative mode
    if (!ps2_mouse_write_ack(0xFF)) {
        display_print("PS/2 Mouse Init Error: Reset Command Failed\n");
    } else {
        // Read BAT code (0xAA) with generous timeout
        uint32_t timeout = 50000;
        uint8_t bat = 0;
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 1) == 1) {
                bat = io_in8(PS2_DATA_PORT);
                break;
            }
        }
        // Read Device ID (0x00)
        timeout = 50000;
        uint8_t id = 0xFF;
        while (timeout--) {
            if ((io_in8(PS2_STATUS_PORT) & 1) == 1) {
                id = io_in8(PS2_DATA_PORT);
                break;
            }
        }

        if (bat == 0xAA && id == 0x00) {
            display_print("PS/2 Mouse Reset OK (Standard 3-Byte mode enforced)\n");
        } else {
            display_print("PS/2 Mouse Reset Warning: Unexpected BAT response\n");
        }
    }

    // 5.1 Configure Hardware Sample Rate (0xF3) to Maximum 200 Hz
    if (!ps2_mouse_write_ack(0xF3) || !ps2_mouse_write_ack(200)) {
        display_print("PS/2 Mouse Init Warning: Set Sample Rate (200Hz) Failed\n");
    } else {
        display_print("PS/2 Mouse Sample Rate set to 200 Hz (Maximum)\n");
    }

    // 5.2 Configure Hardware Resolution (0xE8) to 8 counts/mm (Setting 3)
    if (!ps2_mouse_write_ack(0xE8) || !ps2_mouse_write_ack(3)) {
        display_print("PS/2 Mouse Init Warning: Set Resolution (8 counts/mm) Failed\n");
    } else {
        display_print("PS/2 Mouse Resolution set to 8 counts/mm (High Precision)\n");
    }

    // 5.3 Configure Scaling 1:1 (0xE6) for linear raw counts
    if (!ps2_mouse_write_ack(0xE6)) {
        display_print("PS/2 Mouse Init Warning: Set Scaling 1:1 Failed\n");
    } else {
        display_print("PS/2 Mouse Scaling set to 1:1\n");
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
