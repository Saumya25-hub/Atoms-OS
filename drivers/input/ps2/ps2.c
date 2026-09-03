#include "ps2.h"
#include "mouse.h"
#include <stddef.h>
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/interrupt/include/irq_flags.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT    0x64

static void ps2_wait_write(void) {
    int timeout = 500000;
    while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
        timeout--;
        __asm__ volatile("pause");
    }
}

static void ps2_wait_read(void) {
    int timeout = 500000;
    while (!(io_in8(PS2_STATUS_PORT) & 1) && timeout > 0) {
        timeout--;
        __asm__ volatile("pause");
    }
}

static void ps2_flush_buffer(void) {
    int timeout = 100;
    while ((io_in8(PS2_STATUS_PORT) & 1) && timeout > 0) {
        uint8_t status = io_in8(PS2_STATUS_PORT);
        uint8_t b = io_in8(PS2_DATA_PORT);
        if (status & 0x20) {
            ps2_mouse_handle_byte(b);
        }
        timeout--;
    }
}

void ps2_init(void) {
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0x20); // Read Command Byte
    ps2_wait_read();
    uint8_t config = io_in8(PS2_DATA_PORT);

    // Keep translation enabled (bit 6), enable Keyboard IRQ (bit 0), enable clock (clear bit 4)
    config |= 0x41;
    config &= ~0x10;

    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0x60); // Write Command Byte
    ps2_wait_write();
    io_out8(PS2_DATA_PORT, config);

    // Enable Port 1 (Keyboard)
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0xAE);

    // Enable keyboard scanning (0xF4) and wait for ACK
    ps2_wait_write();
    io_out8(PS2_DATA_PORT, 0xF4);

    // Bounded wait for keyboard ACK (up to ~50ms)
    int wait_ack = 50000;
    while (wait_ack-- > 0) {
        if (io_in8(PS2_STATUS_PORT) & 1) {
            uint8_t ack = io_in8(PS2_DATA_PORT);
            (void)ack;
            break;
        }
        for (volatile int d = 0; d < 10; d++) { io_in8(0x80); }
    }

    ps2_flush_buffer();
}

uint8_t ps2_read_scancode(void) {
    ps2_wait_read();
    return io_in8(PS2_DATA_PORT);
}

// Global instance of the driver
KeyboardDriver ps2_keyboard_driver = {
    .init = ps2_init,
    .read_scancode = ps2_read_scancode
};

// Bounded wait function specifically for keyboard responses while preserving mouse stream
static int ps2_wait_keyboard_response(uint8_t *out_byte, bool *out_valid, PS2RawTrace *trace, int phase) {
    if (out_valid) *out_valid = false;
    uint32_t max_poll = 500000; // ~10ms bounded headroom
    while (max_poll > 0) {
        uint8_t status = io_in8(PS2_STATUS_PORT);
        if (phase == 1 && trace) trace->status_while_waiting1 = status;
        if (phase == 2 && trace) trace->status_while_waiting2 = status;

        if (status & 0x01) { // Output Buffer Full (OBF = 1)
            uint8_t byte = io_in8(PS2_DATA_PORT);

            if (status & 0x20) { // AUX = 1 -> MOUSE BYTE!
                if (phase == 1 && trace) {
                    trace->obf_bit1 = true;
                    trace->aux_bit1 = true;
                    trace->aux_mouse_encountered1 = true;
                    trace->last_mouse_byte1 = byte;
                }
                if (phase == 2 && trace) {
                    trace->obf_bit2 = true;
                    trace->aux_bit2 = true;
                    trace->aux_mouse_encountered2 = true;
                    trace->last_mouse_byte2 = byte;
                }
                // Dispatch mouse byte safely to mouse packet state machine!
                ps2_mouse_handle_byte(byte);
                // Continue waiting for the keyboard response!
                continue;
            }

            // AUX = 0 -> KEYBOARD ORIGIN!
            if (out_byte) *out_byte = byte;
            if (out_valid) *out_valid = true;
            if (phase == 1 && trace) {
                trace->obf_bit1 = true;
                trace->aux_bit1 = false;
                trace->rx_byte1_valid = true;
                trace->rx_byte1 = byte;
            }
            if (phase == 2 && trace) {
                trace->obf_bit2 = true;
                trace->aux_bit2 = false;
                trace->rx_byte2_valid = true;
                trace->rx_byte2 = byte;
            }

            if (byte == 0xFA) return PS2_RESP_ACK;
            if (byte == 0xFE) return PS2_RESP_RESEND;
            if (byte == 0xFC || byte == 0xFD) return PS2_RESP_ERROR;

            // Other scancode - continue waiting
            continue;
        }

        max_poll--;
        __asm__ volatile("pause");
    }

    if (phase == 1 && trace) trace->timeout1 = true;
    if (phase == 2 && trace) trace->timeout2 = true;
    return PS2_RESP_TIMEOUT;
}

bool ps2_keyboard_trace_transaction(uint8_t led_mask, PS2RawTrace *trace) {
    if (!trace) return false;
    for (int i = 0; i < sizeof(PS2RawTrace); i++) {
        ((uint8_t*)trace)[i] = 0;
    }

    irq_flags_t flags = irq_save();

    // 1. STATUS BEFORE DRAIN
    trace->status_before_drain = io_in8(PS2_STATUS_PORT);

    // 2. DRAIN STALE BYTES (Route AUX to mouse handler)
    int drain = 8;
    while ((io_in8(PS2_STATUS_PORT) & 1) && drain > 0) {
        uint8_t s = io_in8(PS2_STATUS_PORT);
        uint8_t b = io_in8(PS2_DATA_PORT);
        if (s & 0x20) {
            ps2_mouse_handle_byte(b);
        }
        if (trace->drained_count < 8) {
            trace->drained_bytes[trace->drained_count++] = b;
        }
        drain--;
    }
    trace->status_after_drain = io_in8(PS2_STATUS_PORT);

    // 3. TX 0xED WITH BOUNDED RESEND LOOP (UP TO 3 RETRIES)
    bool ack1_ok = false;
    for (int retry = 0; retry < 3; retry++) {
        // Wait until IBF == 0
        int timeout = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
            timeout--;
            __asm__ volatile("pause");
        }
        if (timeout == 0) {
            trace->timeout1 = true;
            break;
        }

        // Hardware I/O bus recovery delay (~50us)
        for (volatile int d = 0; d < 50; d++) {
            io_in8(0x80);
        }

        trace->status_before_ed = io_in8(PS2_STATUS_PORT);
        io_out8(PS2_DATA_PORT, 0xED);
        trace->tx_ed_success = true;

        // Wait until controller accepted byte (IBF == 0)
        timeout = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
            timeout--;
            __asm__ volatile("pause");
        }
        trace->status_after_ed_write = io_in8(PS2_STATUS_PORT);

        // Wait for keyboard ACK
        uint8_t ack1_byte = 0;
        bool ack1_valid = false;
        int resp1 = ps2_wait_keyboard_response(&ack1_byte, &ack1_valid, trace, 1);
        if (resp1 == PS2_RESP_ACK && ack1_valid && ack1_byte == 0xFA) {
            ack1_ok = true;
            trace->ack1_is_fa = true;
            break;
        } else if (resp1 == PS2_RESP_RESEND && ack1_valid && ack1_byte == 0xFE) {
            // Keyboard requested retransmission (0xFE NAK)
            trace->resends_observed++;
            // Settling delay before retransmitting same byte
            for (volatile int d = 0; d < 100; d++) {
                io_in8(0x80);
            }
            continue;
        } else {
            break;
        }
    }

    if (!ack1_ok) {
        irq_restore(flags);
        return false;
    }

    // 4. TX LED MASK WITH BOUNDED RESEND LOOP (UP TO 3 RETRIES)
    trace->mask_sent = led_mask & 0x07;
    bool ack2_ok = false;
    for (int retry = 0; retry < 3; retry++) {
        // Wait until IBF == 0
        int timeout = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
            timeout--;
            __asm__ volatile("pause");
        }
        if (timeout == 0) {
            trace->timeout2 = true;
            break;
        }

        // Hardware I/O bus recovery delay (~50us)
        for (volatile int d = 0; d < 50; d++) {
            io_in8(0x80);
        }

        trace->status_before_mask = io_in8(PS2_STATUS_PORT);
        io_out8(PS2_DATA_PORT, trace->mask_sent);

        // Wait until controller accepted byte
        timeout = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && timeout > 0) {
            timeout--;
            __asm__ volatile("pause");
        }
        trace->status_after_mask_write = io_in8(PS2_STATUS_PORT);

        // Wait for keyboard ACK
        uint8_t ack2_byte = 0;
        bool ack2_valid = false;
        int resp2 = ps2_wait_keyboard_response(&ack2_byte, &ack2_valid, trace, 2);
        if (resp2 == PS2_RESP_ACK && ack2_valid && ack2_byte == 0xFA) {
            ack2_ok = true;
            trace->ack2_is_fa = true;
            break;
        } else if (resp2 == PS2_RESP_RESEND && ack2_valid && ack2_byte == 0xFE) {
            // Keyboard requested retransmission (0xFE NAK)
            trace->resends_observed++;
            for (volatile int d = 0; d < 100; d++) {
                io_in8(0x80);
            }
            continue;
        } else {
            break;
        }
    }

    irq_restore(flags);

    trace->final_transaction_pass = (ack1_ok && ack2_ok);
    return trace->final_transaction_pass;
}

bool ps2_keyboard_set_leds(uint8_t led_mask) {
    PS2RawTrace trace;
    for (int retry = 0; retry < 3; retry++) {
        if (ps2_keyboard_trace_transaction(led_mask, &trace)) {
            return true;
        }
    }
    return false;
}

void ps2_run_controller_diag(PS2ControllerDiag *diag) {
    if (!diag) return;
    for (int i = 0; i < sizeof(PS2ControllerDiag); i++) {
        ((uint8_t*)diag)[i] = 0;
    }

    irq_flags_t flags = irq_save();

    // 1. Read Controller Configuration Byte (Command 0x20)
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0x20);
    ps2_wait_read();
    diag->controller_config_byte = io_in8(PS2_DATA_PORT);

    diag->translation_enabled = (diag->controller_config_byte & 0x40) != 0;
    diag->kbd_clock_enabled   = (diag->controller_config_byte & 0x10) == 0;
    diag->mouse_clock_enabled = (diag->controller_config_byte & 0x20) == 0;
    diag->irq1_enabled        = (diag->controller_config_byte & 0x01) != 0;
    diag->irq12_enabled       = (diag->controller_config_byte & 0x02) != 0;

    // 2. Enable Keyboard Port (Command 0xAE)
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0xAE);

    // Settling delay after enabling port
    for (volatile int d = 0; d < 100; d++) { io_in8(0x80); }

    // 3. Test Command 0xEE (PS/2 Echo) with Bounded Resend Handshake
    for (int r = 0; r < 3; r++) {
        ps2_wait_write();
        for (volatile int d = 0; d < 50; d++) { io_in8(0x80); }
        diag->echo_status_before = io_in8(PS2_STATUS_PORT);
        io_out8(PS2_DATA_PORT, 0xEE);
        uint8_t echo_b = 0;
        bool echo_v = false;
        int resp_echo = ps2_wait_keyboard_response(&echo_b, &echo_v, NULL, 0);
        diag->echo_rx_valid = echo_v;
        diag->echo_rx_byte  = echo_b;
        if (resp_echo == PS2_RESP_ACK || echo_b == 0xEE) {
            diag->echo_received = true;
            break;
        }
        if (resp_echo == PS2_RESP_RESEND || echo_b == 0xFE) {
            for (volatile int d = 0; d < 100; d++) { io_in8(0x80); }
            continue;
        }
        break;
    }

    // 4. Test Command 0xF4 (Enable Scanning) with Bounded Resend Handshake
    for (int r = 0; r < 3; r++) {
        ps2_wait_write();
        for (volatile int d = 0; d < 50; d++) { io_in8(0x80); }
        diag->f4_status_before = io_in8(PS2_STATUS_PORT);
        io_out8(PS2_DATA_PORT, 0xF4);
        uint8_t f4_b = 0;
        bool f4_v = false;
        int resp_f4 = ps2_wait_keyboard_response(&f4_b, &f4_v, NULL, 0);
        diag->f4_rx_valid     = f4_v;
        diag->f4_rx_byte      = f4_b;
        if (resp_f4 == PS2_RESP_ACK && f4_b == 0xFA) {
            diag->f4_ack_received = true;
            break;
        }
        if (resp_f4 == PS2_RESP_RESEND || f4_b == 0xFE) {
            for (volatile int d = 0; d < 100; d++) { io_in8(0x80); }
            continue;
        }
        break;
    }

    irq_restore(flags);

    // 5. Test Command 0xED Single Transaction Trace
    ps2_keyboard_trace_transaction(0x02, &diag->ed_trace);
}
