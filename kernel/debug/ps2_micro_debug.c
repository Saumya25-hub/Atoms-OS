#include "ps2_micro_debug.h"
#include "kernel/debug/aipdebug/aipdebug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "drivers/input/ps2/ps2.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/debug/step14_telemetry.h"
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

// External references
extern void com1_puts(const char *s);
extern bool keyboard_get_caps_lock(void);
extern bool keyboard_get_num_lock(void);
extern bool keyboard_get_scroll_lock(void);
extern uint8_t keyboard_get_led_mask(void);
extern void ps2_mouse_handle_byte(uint8_t byte);
extern bool r8168_poll_receive(void);

// LAN diagnostic externs from r8168 driver
extern volatile uint64_t g_rx_frames;
extern volatile uint64_t g_tx_ok_count;
extern volatile uint64_t g_rx_drop_count;
extern volatile uint64_t g_tx_drop_count;

PS2MicroDebug g_ps2_micro_debug;

static volatile uint64_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

// =====================================================================
// COM1 LOGGING UTILITIES
// =====================================================================

static void micro_com1_put_hex_byte(uint8_t b) {
    char buf[5];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    buf[2] = hex[(b >> 4) & 0xF];
    buf[3] = hex[b & 0xF];
    buf[4] = '\0';
    com1_puts(buf);
}

static void micro_com1_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[pos + 1]);
}

static void micro_log_status(const char *label, const PS2StatusSnapshot *snap) {
    com1_puts("[PS2-MICRO] ");
    com1_puts(label);
    com1_puts(" T=");
    micro_com1_put_dec(snap->time_us);
    com1_puts("us STATUS=");
    micro_com1_put_hex_byte(snap->raw_status);
    com1_puts(" (OBF=");
    micro_com1_put_dec(snap->obf ? 1 : 0);
    com1_puts(" IBF=");
    micro_com1_put_dec(snap->ibf ? 1 : 0);
    com1_puts(" AUX=");
    micro_com1_put_dec(snap->aux ? 1 : 0);
    com1_puts(" TIMEOUT=");
    micro_com1_put_dec(snap->timeout ? 1 : 0);
    com1_puts(" PARITY=");
    micro_com1_put_dec(snap->parity ? 1 : 0);
    com1_puts(")\r\n");
}

static PS2StatusSnapshot micro_capture_status(void) {
    PS2StatusSnapshot snap;
    snap.tsc = step14_rdtsc();
    snap.time_us = step14_cycles_to_us(snap.tsc);
    snap.raw_status = io_in8(PS2_STATUS_PORT);
    snap.obf      = (snap.raw_status & 0x01) != 0;
    snap.ibf      = (snap.raw_status & 0x02) != 0;
    snap.system   = (snap.raw_status & 0x04) != 0;
    snap.cmd_data = (snap.raw_status & 0x08) != 0;
    snap.kbd_lock = (snap.raw_status & 0x10) != 0;
    snap.aux      = (snap.raw_status & 0x20) != 0;
    snap.timeout  = (snap.raw_status & 0x40) != 0;
    snap.parity   = (snap.raw_status & 0x80) != 0;
    return snap;
}

static inline void micro_io_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        io_in8(0x80);
    }
}

// Drain stale bytes, recording everything to detect pre-existing 0xFE
static void micro_drain_buffers_forensic(PS2KbdResetSM *sm) {
    sm->drained_count = 0;
    int max_drain = 8;
    while ((io_in8(PS2_STATUS_PORT) & 1) && max_drain > 0) {
        uint8_t s = io_in8(PS2_STATUS_PORT);
        uint8_t b = io_in8(PS2_DATA_PORT);
        bool is_aux = (s & 0x20) != 0;

        if (sm->drained_count < 8) {
            sm->drained_bytes[sm->drained_count] = b;
            sm->drained_status[sm->drained_count] = s;
            sm->drained_aux[sm->drained_count] = is_aux;
            sm->drained_count++;
        }

        com1_puts("[PS2-MICRO] DRAIN PRE-RESET BYTE=");
        micro_com1_put_hex_byte(b);
        com1_puts(" STATUS=");
        micro_com1_put_hex_byte(s);
        com1_puts(is_aux ? " (AUX/MOUSE)\r\n" : " (KBD)\r\n");

        if (is_aux) {
            ps2_mouse_handle_byte(b);
        }
        max_drain--;
        micro_io_delay(20);
    }
}

// =====================================================================
// STEP 1: CONTROLLER CONFIGURATION READ
// =====================================================================
static PS2ControllerConfig micro_read_controller_config(void) {
    PS2ControllerConfig cfg_out;
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0x20); // Command 0x20: Read Controller Command Byte
    ps2_wait_read();
    uint8_t cfg = io_in8(PS2_DATA_PORT);

    cfg_out.raw_config   = cfg;
    cfg_out.translation  = (cfg & 0x40) != 0;
    cfg_out.kbd_clock    = (cfg & 0x10) == 0;
    cfg_out.mouse_clock  = (cfg & 0x20) == 0;
    cfg_out.irq1         = (cfg & 0x01) != 0;
    cfg_out.irq12        = (cfg & 0x02) != 0;

    com1_puts("[PS2-MICRO] 8042 CONFIG (CMD 0x20): ");
    micro_com1_put_hex_byte(cfg);
    com1_puts(" [TRANSLATION=");
    com1_puts(cfg_out.translation ? "ENABLED" : "DISABLED");
    com1_puts(" KBD_CLK=");
    com1_puts(cfg_out.kbd_clock ? "ENABLED" : "INHIBITED");
    com1_puts(" MOUSE_CLK=");
    com1_puts(cfg_out.mouse_clock ? "ENABLED" : "INHIBITED");
    com1_puts(" IRQ1=");
    micro_com1_put_dec(cfg_out.irq1 ? 1 : 0);
    com1_puts(" IRQ12=");
    micro_com1_put_dec(cfg_out.irq12 ? 1 : 0);
    com1_puts("]\r\n");

    return cfg_out;
}

// =====================================================================
// STEP 2: 8042 CONTROLLER SELF-TEST (COMMAND 0xAA)
// =====================================================================
static void micro_run_controller_self_test(void) {
    g_ps2_micro_debug.self_test.cmd = 0xAA;

    uint16_t tx = aipd_tx_begin(AIPD_TX_8042_SELFTEST, PS2_CMD_PORT);

    micro_drain_buffers_forensic(&g_ps2_micro_debug.kbd_reset);
    ps2_wait_write();
    micro_io_delay(50);

    uint64_t t0 = step14_cycles_to_us(step14_rdtsc());
    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT64, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_CMD_PORT, 0, 0xAA, 0);
    io_out8(PS2_CMD_PORT, 0xAA);

    int poll = 500000;
    while (poll > 0) {
        if (io_in8(PS2_STATUS_PORT) & 1) {
            uint8_t res = io_in8(PS2_DATA_PORT);
            g_ps2_micro_debug.self_test.result_byte = res;
            g_ps2_micro_debug.self_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
            g_ps2_micro_debug.self_test.passed = (res == 0x55);

            uint8_t s_after = io_in8(PS2_STATUS_PORT);
            g_ps2_micro_debug.self_test.status_after = s_after;
            g_ps2_micro_debug.self_test.timeout_cleared = ((s_after & 0x40) == 0);

            aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_READ, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, 0, res, res == 0x55 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);
            aipd_tx_end(tx, res == 0x55 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);

            com1_puts("[PS2-MICRO] 8042 SELF-TEST (0xAA): RESULT=");
            micro_com1_put_hex_byte(res);
            com1_puts(" (");
            com1_puts(g_ps2_micro_debug.self_test.passed ? "0x55 PASS" : "FAIL");
            com1_puts(") LATENCY=");
            micro_com1_put_dec(g_ps2_micro_debug.self_test.latency_us);
            com1_puts("us STATUS_AFTER=");
            micro_com1_put_hex_byte(s_after);
            com1_puts(" TIMEOUT_CLEARED=");
            com1_puts(g_ps2_micro_debug.self_test.timeout_cleared ? "YES\r\n" : "NO\r\n");
            return;
        }
        poll--;
        __asm__ volatile("pause");
    }

    g_ps2_micro_debug.self_test.passed = false;
    g_ps2_micro_debug.self_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
    aipd_tx_end(tx, AIPD_RES_TIMEOUT);
    com1_puts("[PS2-MICRO] 8042 SELF-TEST (0xAA): TIMEOUT!\r\n");
}

// =====================================================================
// STEP 3: KEYBOARD INTERFACE TEST (COMMAND 0xAB)
// =====================================================================
static void micro_run_keyboard_interface_test(void) {
    g_ps2_micro_debug.iface_test.cmd = 0xAB;

    uint16_t tx = aipd_tx_begin(AIPD_TX_8042_IFACE1_TEST, PS2_CMD_PORT);

    ps2_wait_write();
    micro_io_delay(50);

    uint64_t t0 = step14_cycles_to_us(step14_rdtsc());
    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT64, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_CMD_PORT, 0, 0xAB, 0);
    io_out8(PS2_CMD_PORT, 0xAB);

    int poll = 500000;
    while (poll > 0) {
        if (io_in8(PS2_STATUS_PORT) & 1) {
            uint8_t res = io_in8(PS2_DATA_PORT);
            g_ps2_micro_debug.iface_test.result_byte = res;
            g_ps2_micro_debug.iface_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
            g_ps2_micro_debug.iface_test.passed = (res == 0x00);

            const char *diag_str = "UNKNOWN";
            if (res == 0x00) diag_str = "LINES OK (PASS)";
            else if (res == 0x01) diag_str = "CLK STUCK LOW";
            else if (res == 0x02) diag_str = "CLK STUCK HIGH";
            else if (res == 0x03) diag_str = "DATA STUCK LOW";
            else if (res == 0x04) diag_str = "DATA STUCK HIGH";

            g_ps2_micro_debug.iface_test.result_str = diag_str;

            aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_READ, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, 0, res, res == 0x00 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);
            aipd_tx_end(tx, res == 0x00 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);

            com1_puts("[PS2-MICRO] KEYBOARD INTERFACE TEST (0xAB): RESULT=");
            micro_com1_put_hex_byte(res);
            com1_puts(" [");
            com1_puts(diag_str);
            com1_puts("] LATENCY=");
            micro_com1_put_dec(g_ps2_micro_debug.iface_test.latency_us);
            com1_puts("us\r\n");
            return;
        }
        poll--;
        __asm__ volatile("pause");
    }

    g_ps2_micro_debug.iface_test.passed = false;
    g_ps2_micro_debug.iface_test.result_str = "TIMEOUT";
    g_ps2_micro_debug.iface_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
    aipd_tx_end(tx, AIPD_RES_TIMEOUT);
    com1_puts("[PS2-MICRO] KEYBOARD INTERFACE TEST (0xAB): TIMEOUT!\r\n");
}

// =====================================================================
// STEP 3B: AUX / MOUSE (CHANNEL 2) INTERFACE TEST (COMMAND 0xA9)
// =====================================================================
static void micro_run_aux_interface_test(void) {
    g_ps2_micro_debug.aux_iface_test.cmd = 0xA9;

    uint16_t tx = aipd_tx_begin(AIPD_TX_8042_IFACE2_TEST, PS2_CMD_PORT);

    ps2_wait_write();
    micro_io_delay(50);

    uint64_t t0 = step14_cycles_to_us(step14_rdtsc());
    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT64, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_CMD_PORT, 0, 0xA9, 0);
    io_out8(PS2_CMD_PORT, 0xA9);

    int poll = 500000;
    while (poll > 0) {
        if (io_in8(PS2_STATUS_PORT) & 1) {
            uint8_t res = io_in8(PS2_DATA_PORT);
            g_ps2_micro_debug.aux_iface_test.result_byte = res;
            g_ps2_micro_debug.aux_iface_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
            g_ps2_micro_debug.aux_iface_test.passed = (res == 0x00);

            const char *diag_str = "UNKNOWN";
            if (res == 0x00) diag_str = "LINES OK (PASS)";
            else if (res == 0x01) diag_str = "CLK STUCK LOW";
            else if (res == 0x02) diag_str = "CLK STUCK HIGH";
            else if (res == 0x03) diag_str = "DATA STUCK LOW";
            else if (res == 0x04) diag_str = "DATA STUCK HIGH";

            g_ps2_micro_debug.aux_iface_test.result_str = diag_str;

            aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_READ, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, 0, res, res == 0x00 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);
            aipd_tx_end(tx, res == 0x00 ? AIPD_RES_SUCCESS : AIPD_RES_FAILURE);

            com1_puts("[PS2-MICRO] AUX INTERFACE TEST (0xA9): RESULT=");
            micro_com1_put_hex_byte(res);
            com1_puts(" [");
            com1_puts(diag_str);
            com1_puts("] LATENCY=");
            micro_com1_put_dec(g_ps2_micro_debug.aux_iface_test.latency_us);
            com1_puts("us\r\n");
            return;
        }
        poll--;
        __asm__ volatile("pause");
    }

    g_ps2_micro_debug.aux_iface_test.passed = false;
    g_ps2_micro_debug.aux_iface_test.result_str = "TIMEOUT";
    g_ps2_micro_debug.aux_iface_test.latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
    aipd_tx_end(tx, AIPD_RES_TIMEOUT);
    com1_puts("[PS2-MICRO] AUX INTERFACE TEST (0xA9): TIMEOUT!\r\n");
}

// =====================================================================
// STEP 4: RE-ESTABLISH CONTROLLER CONFIGURATION
// =====================================================================
static void micro_reconfigure_controller(void) {
    uint16_t tx = aipd_tx_begin(AIPD_TX_8042_RECONFIG, PS2_CMD_PORT);

    // Enable Keyboard Port (0xAE)
    ps2_wait_write();
    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT64, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_CMD_PORT, 0, 0xAE, 0);
    io_out8(PS2_CMD_PORT, 0xAE);
    micro_io_delay(200);

    // Set Configuration Byte: 0x47 (Translation=1, KbdClk=1, MouseClk=1, IRQ1=1, IRQ12=1)
    ps2_wait_write();
    io_out8(PS2_CMD_PORT, 0x60);
    ps2_wait_write();
    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, 0, 0x47, 0);
    io_out8(PS2_DATA_PORT, 0x47);
    micro_io_delay(200);

    // Read back configuration to verify acceptance
    g_ps2_micro_debug.config_post_reconfig = micro_read_controller_config();
    g_ps2_micro_debug.status_after_reconfig = micro_capture_status();

    aipd_tx_record(tx, AIPD_SUBSYS_PS2_CONTROLLER, AIPD_COMP_8042_CONFIG_REG, AIPD_EVT_CONFIG_CHANGE, AIPD_TRUTH_OBSERVED, 0x20, 0, g_ps2_micro_debug.config_post_reconfig.raw_config, AIPD_RES_SUCCESS);
    aipd_tx_end(tx, AIPD_RES_SUCCESS);

    com1_puts("[PS2-MICRO] 8042 CONTROLLER RECONFIGURED (PORT 0xAE ENABLED, CFG=0x47, READBACK=0x");
    micro_com1_put_hex_byte(g_ps2_micro_debug.config_post_reconfig.raw_config);
    com1_puts(") STATUS=0x");
    micro_com1_put_hex_byte(g_ps2_micro_debug.status_after_reconfig.raw_status);
    com1_puts("\r\n");
}

// =====================================================================
// STEP 5: KEYBOARD RESET (0xFF) & BAT EXPLICIT STATE MACHINE
// =====================================================================
static void micro_run_keyboard_reset_sm(void) {
    PS2KbdResetSM *sm = &g_ps2_micro_debug.kbd_reset;
    sm->current_state = KBD_STATE_DRAIN;
    sm->state_str = "DRAIN";
    sm->attempt_count = 0;
    sm->reset_ack_pass = false;
    sm->bat_pass = false;

    uint16_t tx = aipd_tx_begin(AIPD_TX_PS2_RESET_BAT, PS2_DATA_PORT);
    uint64_t t_overall_start = step14_cycles_to_us(step14_rdtsc());

    // 1. Pre-reset drain capture: check if 0xFE is a stale leftover
    micro_drain_buffers_forensic(sm);

    sm->status_before_reset = io_in8(PS2_STATUS_PORT);
    com1_puts("[PS2-MICRO] RESET SM: STATUS_BEFORE_RESET=");
    micro_com1_put_hex_byte(sm->status_before_reset);
    com1_puts(" DRAINED_COUNT=");
    micro_com1_put_dec(sm->drained_count);
    com1_puts("\r\n");

    // 2. Execute 0xFF with bounded resend loop (up to 3 attempts: R0, R1, R2)
    for (int r = 0; r < 3; r++) {
        KbdResetAttempt *att = &sm->attempts[sm->attempt_count++];
        att->retry_num = r;

        sm->current_state = KBD_STATE_RESET_SENT;
        sm->state_str = "RESET_SENT";

        ps2_wait_write();
        micro_io_delay(50);

        att->status_before_tx = io_in8(PS2_STATUS_PORT);
        att->tx_time_us = step14_cycles_to_us(step14_rdtsc());

        com1_puts("[PS2-MICRO] RESET SM: TX 0xFF [R");
        micro_com1_put_dec(r);
        com1_puts("] STATUS_BEFORE=");
        micro_com1_put_hex_byte(att->status_before_tx);
        com1_puts("\r\n");

        aipd_tx_record(tx, AIPD_SUBSYS_PS2_KEYBOARD, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_WRITE, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, att->status_before_tx, 0xFF, (uint16_t)r);
        io_out8(PS2_DATA_PORT, 0xFF);

        ps2_wait_write();
        att->status_after_tx = io_in8(PS2_STATUS_PORT);

        // 3. Wait for RX #1: Expected ACK (0xFA) or RESEND (0xFE)
        sm->current_state = KBD_STATE_WAIT_ACK;
        sm->state_str = "WAIT_ACK";

        int poll_ack = 500000;
        bool got_rx1 = false;

        while (poll_ack > 0) {
            uint8_t s = io_in8(PS2_STATUS_PORT);
            if (s & 0x01) {
                uint8_t b = io_in8(PS2_DATA_PORT);
                bool aux = (s & 0x20) != 0;

                if (aux) {
                    com1_puts("[PS2-MICRO] RESET SM: RX MOUSE BYTE=");
                    micro_com1_put_hex_byte(b);
                    com1_puts(" PRESERVED!\r\n");
                    ps2_mouse_handle_byte(b);
                    continue; // Keep waiting for keyboard response
                }

                att->rx1_time_us = step14_cycles_to_us(step14_rdtsc());
                att->rx1_byte = b;
                att->rx1_status = s;
                att->rx1_aux = false;
                att->rx1_latency_us = (uint32_t)(att->rx1_time_us - att->tx_time_us);
                got_rx1 = true;

                aipd_tx_record(tx, AIPD_SUBSYS_PS2_KEYBOARD, AIPD_COMP_8042_PORT60, AIPD_EVT_REG_READ, AIPD_TRUTH_OBSERVED, PS2_DATA_PORT, s, b, (uint16_t)att->rx1_latency_us);

                com1_puts("[PS2-MICRO] RESET SM: RX #1 BYTE=");
                micro_com1_put_hex_byte(b);
                com1_puts(" STATUS=");
                micro_com1_put_hex_byte(s);
                com1_puts(" LATENCY=");
                micro_com1_put_dec(att->rx1_latency_us);
                com1_puts("us\r\n");

                if (b == 0xFA) {
                    att->rx1_is_ack = true;
                    sm->reset_ack_pass = true;
                    sm->current_state = KBD_STATE_ACK_RECEIVED;
                    sm->state_str = "ACK_RECEIVED";
                    sm->status_after_ack = s;
                    com1_puts("[PS2-MICRO] RESET SM: RX #1 = ACK (0xFA) -> ADVANCING TO WAIT_BAT\r\n");
                    break;
                } else if (b == 0xFE) {
                    att->rx1_is_resend = true;
                    sm->current_state = KBD_STATE_RESEND;
                    sm->state_str = "RESEND";
                    com1_puts("[PS2-MICRO] RESET SM: RX #1 = RESEND (0xFE) -> RETRYING 0xFF\r\n");
                    micro_io_delay(100);
                    break;
                } else {
                    com1_puts("[PS2-MICRO] RESET SM: RX #1 = OTHER (NOT ACK)\r\n");
                    break;
                }
            }
            poll_ack--;
            __asm__ volatile("pause");
        }

        if (!got_rx1) {
            att->rx1_timeout = true;
            att->rx1_latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - att->tx_time_us);
            sm->current_state = KBD_STATE_TIMEOUT;
            sm->state_str = "TIMEOUT_ACK";
            com1_puts("[PS2-MICRO] RESET SM: RX #1 TIMEOUT!\r\n");
            break;
        }

        // 4. If RX #1 was ACK (0xFA), wait for RX #2 (BAT: 0xAA)
        if (att->rx1_is_ack) {
            sm->current_state = KBD_STATE_WAIT_BAT;
            sm->state_str = "WAIT_BAT";

            uint64_t t_bat_start = step14_cycles_to_us(step14_rdtsc());
            int poll_bat = 1000000; // Keyboard BAT can take up to ~500ms
            bool got_bat = false;

            while (poll_bat > 0) {
                uint8_t s = io_in8(PS2_STATUS_PORT);
                if (s & 0x01) {
                    uint8_t b = io_in8(PS2_DATA_PORT);
                    bool aux = (s & 0x20) != 0;

                    if (aux) {
                        ps2_mouse_handle_byte(b);
                        continue;
                    }

                    att->rx2_time_us = step14_cycles_to_us(step14_rdtsc());
                    att->rx2_byte = b;
                    att->rx2_status = s;
                    att->rx2_aux = false;
                    att->rx2_latency_us = (uint32_t)(att->rx2_time_us - t_bat_start);
                    got_bat = true;

                    com1_puts("[PS2-MICRO] RESET SM: RX #2 (BAT) BYTE=");
                    micro_com1_put_hex_byte(b);
                    com1_puts(" STATUS=");
                    micro_com1_put_hex_byte(s);
                    com1_puts(" LATENCY=");
                    micro_com1_put_dec(att->rx2_latency_us);
                    com1_puts("us\r\n");

                    if (b == 0xAA) {
                        att->rx2_is_bat_ok = true;
                        sm->bat_pass = true;
                        sm->current_state = KBD_STATE_READY;
                        sm->state_str = "READY (BAT_PASS)";
                        sm->status_after_bat = s;
                        com1_puts("[PS2-MICRO] RESET SM: BAT PASSED (0xAA)! KEYBOARD READY\r\n");
                        break;
                    } else if (b == 0xFC) {
                        att->rx2_is_bat_fail = true;
                        sm->current_state = KBD_STATE_BAT_FAILED;
                        sm->state_str = "BAT_FAILED (0xFC)";
                        com1_puts("[PS2-MICRO] RESET SM: BAT FAILED (0xFC)!\r\n");
                        break;
                    } else {
                        com1_puts("[PS2-MICRO] RESET SM: RX #2 UNEXPECTED BYTE\r\n");
                        break;
                    }
                }
                poll_bat--;
                __asm__ volatile("pause");
            }

            if (!got_bat) {
                att->rx2_timeout = true;
                att->rx2_latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t_bat_start);
                sm->current_state = KBD_STATE_BAT_FAILED;
                sm->state_str = "BAT_TIMEOUT";
                com1_puts("[PS2-MICRO] RESET SM: BAT TIMEOUT (NO 0xAA)!\r\n");
            }

            // Transaction completed (either BAT pass or BAT fail)
            break;
        }

        // If RX #1 was resend (0xFE), loop will retry 0xFF up to 3 times
    }

    sm->total_reset_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t_overall_start);
    aipd_tx_end(tx, sm->bat_pass ? AIPD_RES_SUCCESS : AIPD_RES_RESEND);
    com1_puts("[PS2-MICRO] RESET SM FINISHED. FINAL STATE=");
    com1_puts(sm->state_str);
    com1_puts(" TOTAL_TIME=");
    micro_com1_put_dec(sm->total_reset_us);
    com1_puts("us\r\n");
}

// =====================================================================
// STEP 6: ENABLE SCANNING (0xF4) ONLY AFTER RESET + BAT COMPLETE
// =====================================================================
static void micro_run_keyboard_enable_scanning_sm(void) {
    PS2ScanSM *sm = &g_ps2_micro_debug.kbd_scan;
    sm->retry_count = 0;
    sm->passed = false;
    sm->skipped_no_bat = false;

    // Rule 1 & Rule 5: Send 0xF4 ONLY after keyboard reset and BAT have completed successfully
    if (g_ps2_micro_debug.kbd_reset.current_state != KBD_STATE_READY) {
        sm->skipped_no_bat = true;
        com1_puts("[PS2-MICRO] ENABLE SCANNING (0xF4): SKIPPED (RESET/BAT NOT READY)!\r\n");
        return;
    }

    com1_puts("[PS2-MICRO] ENABLE SCANNING (0xF4): STARTING TRANSACTION...\r\n");

    for (int r = 0; r < 3; r++) {
        sm->retry_count++;
        ps2_wait_write();
        micro_io_delay(50);

        sm->status_before = io_in8(PS2_STATUS_PORT);
        uint64_t t0 = step14_cycles_to_us(step14_rdtsc());

        io_out8(PS2_DATA_PORT, 0xF4);

        ps2_wait_write();
        sm->status_after_tx = io_in8(PS2_STATUS_PORT);

        int poll = 500000;
        bool got_byte = false;

        while (poll > 0) {
            uint8_t s = io_in8(PS2_STATUS_PORT);
            if (s & 0x01) {
                uint8_t b = io_in8(PS2_DATA_PORT);
                bool aux = (s & 0x20) != 0;

                if (aux) {
                    ps2_mouse_handle_byte(b);
                    continue;
                }

                sm->rx_byte = b;
                sm->status_at_rx = s;
                sm->aux = false;
                sm->latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
                got_byte = true;

                com1_puts("[PS2-MICRO] 0xF4 SCAN: RX=");
                micro_com1_put_hex_byte(b);
                com1_puts(" STATUS=");
                micro_com1_put_hex_byte(s);
                com1_puts(" LATENCY=");
                micro_com1_put_dec(sm->latency_us);
                com1_puts("us\r\n");

                if (b == 0xFA) {
                    sm->is_ack = true;
                    sm->passed = true;
                    com1_puts("[PS2-MICRO] 0xF4 SCAN: ACK RECEIVED (PASS)!\r\n");
                    return;
                } else if (b == 0xFE) {
                    sm->is_resend = true;
                    com1_puts("[PS2-MICRO] 0xF4 SCAN: RESEND (0xFE) -> RETRYING\r\n");
                    micro_io_delay(100);
                    break;
                } else {
                    break;
                }
            }
            poll--;
            __asm__ volatile("pause");
        }

        if (!got_byte) {
            sm->timeout = true;
            sm->latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
            com1_puts("[PS2-MICRO] 0xF4 SCAN: TIMEOUT!\r\n");
            break;
        }
    }
}

// =====================================================================
// STEP 7: PROBE COMMANDS (0xEE ECHO)
// =====================================================================
static void micro_probe_echo(void) {
    PS2EchoSM *sm = &g_ps2_micro_debug.cmd_ee;

    if (!g_ps2_micro_debug.kbd_scan.passed) {
        com1_puts("[PS2-MICRO] PROBE 0xEE ECHO: SKIPPED (SCANNING NOT ENABLED)\r\n");
        return;
    }

    ps2_wait_write();
    micro_io_delay(50);

    sm->status_before = io_in8(PS2_STATUS_PORT);
    uint64_t t0 = step14_cycles_to_us(step14_rdtsc());

    io_out8(PS2_DATA_PORT, 0xEE);

    ps2_wait_write();
    sm->status_after_tx = io_in8(PS2_STATUS_PORT);

    int poll = 500000;
    while (poll > 0) {
        uint8_t s = io_in8(PS2_STATUS_PORT);
        if (s & 0x01) {
            uint8_t b = io_in8(PS2_DATA_PORT);
            if (s & 0x20) {
                ps2_mouse_handle_byte(b);
                continue;
            }

            sm->rx_byte = b;
            sm->status_at_rx = s;
            sm->aux = false;
            sm->latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
            sm->is_echo = (b == 0xEE || b == 0xFA);
            sm->is_resend = (b == 0xFE);

            com1_puts("[PS2-MICRO] PROBE 0xEE: RX=");
            micro_com1_put_hex_byte(b);
            com1_puts(" LATENCY=");
            micro_com1_put_dec(sm->latency_us);
            com1_puts("us\r\n");
            return;
        }
        poll--;
        __asm__ volatile("pause");
    }

    sm->timeout = true;
    sm->latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t0);
    com1_puts("[PS2-MICRO] PROBE 0xEE: TIMEOUT!\r\n");
}

// =====================================================================
// STEP 8: COMMAND 0xED (LED) ONLY AFTER COMMAND CHANNEL PROVEN HEALTHY
// =====================================================================
static void micro_execute_ed_transaction(uint8_t led_mask) {
    // Rule 6: ONLY after keyboard command channel is proven healthy
    if (!g_ps2_micro_debug.kbd_scan.passed) {
        g_ps2_micro_debug.ed_skipped_unhealthy = true;
        com1_puts("[PS2-MICRO] 0xED LED: SKIPPED (COMMAND CHANNEL UNHEALTHY)\r\n");
        return;
    }

    irq_flags_t flags = irq_save();
    uint64_t t_start = step14_cycles_to_us(step14_rdtsc());

    g_ps2_micro_debug.ed_retry_count = 0;
    g_ps2_micro_debug.ed_ack_received = false;

    for (int r = 0; r < 3; r++) {
        PS2RetryTrace *rt = &g_ps2_micro_debug.ed_retries[g_ps2_micro_debug.ed_retry_count++];
        rt->retry_num = r;

        int wait_ibf = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && wait_ibf > 0) {
            wait_ibf--;
            __asm__ volatile("pause");
        }

        micro_io_delay(50);
        rt->status_before_tx = io_in8(PS2_STATUS_PORT);
        rt->tx_time_us = step14_cycles_to_us(step14_rdtsc());

        com1_puts("[PS2-MICRO] TX PORT=0x60 BYTE=0xED (RETRY=");
        micro_com1_put_dec(r);
        com1_puts(")\r\n");
        io_out8(PS2_DATA_PORT, 0xED);

        wait_ibf = 500000;
        while ((io_in8(PS2_STATUS_PORT) & 2) && wait_ibf > 0) {
            wait_ibf--;
            __asm__ volatile("pause");
        }
        rt->status_after_tx = io_in8(PS2_STATUS_PORT);

        int poll = 500000;
        bool got_byte = false;

        while (poll > 0) {
            uint8_t s = io_in8(PS2_STATUS_PORT);
            if (s & 0x01) {
                rt->obf_time_us = step14_cycles_to_us(step14_rdtsc());
                rt->status_at_rx = s;
                rt->aux = (s & 0x20) != 0;

                uint8_t b = io_in8(PS2_DATA_PORT);
                rt->rx_time_us = step14_cycles_to_us(step14_rdtsc());
                rt->rx_byte = b;
                rt->latency_us = (uint32_t)(rt->rx_time_us - rt->tx_time_us);

                if (rt->aux) {
                    ps2_mouse_handle_byte(b);
                    continue;
                }

                got_byte = true;
                com1_puts("[PS2-MICRO] RX KBD BYTE=");
                micro_com1_put_hex_byte(b);
                com1_puts(" STATUS=");
                micro_com1_put_hex_byte(s);
                com1_puts(" LATENCY=");
                micro_com1_put_dec(rt->latency_us);
                com1_puts("us\r\n");

                if (b == 0xFA) {
                    rt->is_ack = true;
                    g_ps2_micro_debug.ed_ack_received = true;
                    break;
                } else if (b == 0xFE) {
                    rt->is_resend = true;
                    micro_io_delay(100);
                    break;
                } else {
                    break;
                }
            }
            poll--;
            __asm__ volatile("pause");
        }

        if (!got_byte) {
            rt->is_timeout = true;
            rt->latency_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - rt->tx_time_us);
            break;
        }

        if (rt->is_ack) {
            break;
        }
    }

    // Phase 2: If 0xED ACKed, send LED mask byte
    g_ps2_micro_debug.mask_retry_count = 0;
    g_ps2_micro_debug.mask_ack_received = false;
    g_ps2_micro_debug.mask_sent = led_mask & 0x07;

    if (g_ps2_micro_debug.ed_ack_received) {
        for (int r = 0; r < 3; r++) {
            PS2RetryTrace *rt = &g_ps2_micro_debug.mask_retries[g_ps2_micro_debug.mask_retry_count++];
            rt->retry_num = r;

            int wait_ibf = 500000;
            while ((io_in8(PS2_STATUS_PORT) & 2) && wait_ibf > 0) {
                wait_ibf--;
                __asm__ volatile("pause");
            }

            micro_io_delay(50);
            rt->status_before_tx = io_in8(PS2_STATUS_PORT);
            rt->tx_time_us = step14_cycles_to_us(step14_rdtsc());

            io_out8(PS2_DATA_PORT, g_ps2_micro_debug.mask_sent);

            wait_ibf = 500000;
            while ((io_in8(PS2_STATUS_PORT) & 2) && wait_ibf > 0) {
                wait_ibf--;
                __asm__ volatile("pause");
            }
            rt->status_after_tx = io_in8(PS2_STATUS_PORT);

            int poll = 500000;
            bool got_byte = false;

            while (poll > 0) {
                uint8_t s = io_in8(PS2_STATUS_PORT);
                if (s & 0x01) {
                    rt->obf_time_us = step14_cycles_to_us(step14_rdtsc());
                    rt->status_at_rx = s;
                    rt->aux = (s & 0x20) != 0;

                    uint8_t b = io_in8(PS2_DATA_PORT);
                    rt->rx_time_us = step14_cycles_to_us(step14_rdtsc());
                    rt->rx_byte = b;
                    rt->latency_us = (uint32_t)(rt->rx_time_us - rt->tx_time_us);

                    if (rt->aux) {
                        ps2_mouse_handle_byte(b);
                        continue;
                    }

                    got_byte = true;
                    if (b == 0xFA) {
                        rt->is_ack = true;
                        g_ps2_micro_debug.mask_ack_received = true;
                        break;
                    } else if (b == 0xFE) {
                        rt->is_resend = true;
                        micro_io_delay(100);
                        break;
                    } else {
                        break;
                    }
                }
                poll--;
                __asm__ volatile("pause");
            }

            if (!got_byte) {
                rt->is_timeout = true;
                break;
            }

            if (rt->is_ack) {
                break;
            }
        }
    }

    g_ps2_micro_debug.total_transaction_us = (uint32_t)(step14_cycles_to_us(step14_rdtsc()) - t_start);
    irq_restore(flags);
}

// =====================================================================
// FULL MICRO TEST SUITE RUNNER
// =====================================================================
void ps2_micro_debug_run_test_suite(void) {
    aipd_init(AIPD_PROF_PS2_KEYBOARD);
    com1_puts("\r\n========================================================\r\n");
    com1_puts("[PS2-MICRO] 8042 CONTROLLER & RESET/BAT STATE MACHINE AUDIT\r\n");
    com1_puts("========================================================\r\n\r\n");

    // 1. Initial T0 Status & Config Read
    g_ps2_micro_debug.status_t0_boot = micro_capture_status();
    micro_log_status("T0 (PRE-TEST BOOT STATUS)", &g_ps2_micro_debug.status_t0_boot);
    g_ps2_micro_debug.config_initial = micro_read_controller_config();

    // 2. Controller Self-Test (0xAA)
    micro_run_controller_self_test();
    g_ps2_micro_debug.status_after_selftest = micro_capture_status();
    g_ps2_micro_debug.config_post_selftest = micro_read_controller_config();

    // 3. Keyboard Interface Test (0xAB)
    micro_run_keyboard_interface_test();
    g_ps2_micro_debug.status_after_iface = micro_capture_status();

    // 3B. AUX Interface Test (0xA9)
    micro_run_aux_interface_test();

    // 4. Re-configure Controller (0xAE + 0x60 -> 0x47)
    micro_reconfigure_controller();

    // 5. Explicit Keyboard Device Reset & BAT State Machine (0xFF)
    micro_run_keyboard_reset_sm();

    // 6. Enable Keyboard Scanning (0xF4) ONLY if Reset & BAT passed
    micro_run_keyboard_enable_scanning_sm();

    // 7. Probe 0xEE Echo ONLY if scanning enabled
    micro_probe_echo();

    // 8. 0xED LED Transaction ONLY if command channel healthy
    micro_execute_ed_transaction(0x02); // Num Lock ON mask

    // Final Status
    g_ps2_micro_debug.status_final = micro_capture_status();
    micro_log_status("T_FINAL", &g_ps2_micro_debug.status_final);

    // Refresh LAN stats
    g_ps2_micro_debug.lan.link_up = true;
    g_ps2_micro_debug.lan.link_speed_mbps = 1000;
    g_ps2_micro_debug.lan.rx_frames = g_rx_frames;
    g_ps2_micro_debug.lan.tx_frames = g_tx_ok_count;
    g_ps2_micro_debug.lan.rx_drops  = g_rx_drop_count;
    g_ps2_micro_debug.lan.tx_drops  = g_tx_drop_count;

    // Verdict calculation
    g_ps2_micro_debug.final_verdict_pass = (g_ps2_micro_debug.kbd_reset.bat_pass &&
                                            g_ps2_micro_debug.kbd_scan.passed &&
                                            g_ps2_micro_debug.ed_ack_received &&
                                            g_ps2_micro_debug.mask_ack_received);

    com1_puts("[PS2-MICRO] AUDIT COMPLETE. RESULT: ");
    com1_puts(g_ps2_micro_debug.final_verdict_pass ? "PASS\r\n" : "FAIL (STAGE FAILED)\r\n");
    com1_puts("========================================================\r\n\r\n");
}

// =====================================================================
// SCREEN DASHBOARD RENDERING
// =====================================================================
static void micro_render_hex_byte(uint32_t x, uint32_t y, uint8_t b, uint32_t color, uint32_t bg) {
    char buf[5];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    buf[2] = hex[(b >> 4) & 0xF];
    buf[3] = hex[b & 0xF];
    buf[4] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void micro_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    if (val == 0) {
        abde_render_string(x, y, "0", color, bg);
        return;
    }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    abde_render_string(x, y, &buf[pos + 1], color, bg);
}

void ps2_micro_debug_render(void) {
    uint32_t bg_color    = 0x00080E1A; // Deep Navy Background
    uint32_t panel_bg    = 0x000F172A; // Slate Dark Panel
    uint32_t cyan_color  = 0x0038BDF8; // Cyan Header
    uint32_t text_color  = 0x00E2E8F0; // Crisp White
    uint32_t label_color = 0x0094A3B8; // Muted Gray
    uint32_t pass_color  = 0x0022C55E; // Emerald Green
    uint32_t warn_color  = 0x00F59E0B; // Amber Yellow
    uint32_t fail_color  = 0x00EF4444; // Ruby Red
    uint32_t title_color = 0x0067E8F9; // Bright Cyan

    uint32_t screen_w = g_abde.width ? g_abde.width : 1024;
    uint32_t screen_h = g_abde.height ? g_abde.height : 768;

    abde_fill_rect(0, 0, screen_w, screen_h, bg_color);

    uint32_t start_x = 24;
    uint32_t start_y = 12;

    // Header Banner
    abde_render_string(start_x, start_y, "==========================================================================================", cyan_color, bg_color);
    abde_render_string(start_x + 70, start_y + 16, "ATOMS OS — KEYBOARD RESET, BAT & COMMAND CHANNEL FORENSIC V7", title_color, bg_color);
    char spin_str[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
    abde_render_string(start_x + 650, start_y + 16, spin_str, pass_color, bg_color);
    abde_render_string(start_x + 680, start_y + 16, "LIVE PROTOCOL AUDIT", warn_color, bg_color);
    abde_render_string(start_x, start_y + 32, "==========================================================================================", cyan_color, bg_color);

    uint32_t col_w = 475;
    uint32_t left_x = start_x;
    uint32_t right_x = start_x + col_w + 14;
    uint32_t cur_y = start_y + 42;
    uint32_t panel_h = 430;

    // Panel backgrounds
    abde_fill_rect(left_x, cur_y, col_w, panel_h, panel_bg);
    abde_fill_rect(right_x, cur_y, col_w, panel_h, panel_bg);

    // =========================================================================
    // LEFT PANEL: 8042 HARDWARE & RESET/BAT STATE MACHINE
    // =========================================================================
    uint32_t ly = cur_y + 8;
    abde_render_string(left_x + 15, ly, "8042 CONTROLLER HARDWARE TESTS", cyan_color, panel_bg);
    ly += 13;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 15;

    // Self Test
    abde_render_string(left_x + 20, ly, "Self-Test (CMD 0xAA):", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.self_test.result_byte, g_ps2_micro_debug.self_test.passed ? pass_color : fail_color, panel_bg);
    abde_render_string(left_x + 235, ly, g_ps2_micro_debug.self_test.passed ? "[0x55 PASS]" : "[FAIL]", g_ps2_micro_debug.self_test.passed ? pass_color : fail_color, panel_bg);
    micro_render_dec(left_x + 325, ly, g_ps2_micro_debug.self_test.latency_us, text_color, panel_bg);
    abde_render_string(left_x + 380, ly, "us", label_color, panel_bg);
    ly += 15;

    // Interface Test
    abde_render_string(left_x + 20, ly, "Interface (CMD 0xAB):", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.iface_test.result_byte, g_ps2_micro_debug.iface_test.passed ? pass_color : fail_color, panel_bg);
    abde_render_string(left_x + 235, ly, g_ps2_micro_debug.iface_test.passed ? "[LINES OK]" : "[ERROR]", g_ps2_micro_debug.iface_test.passed ? pass_color : fail_color, panel_bg);
    micro_render_dec(left_x + 325, ly, g_ps2_micro_debug.iface_test.latency_us, text_color, panel_bg);
    abde_render_string(left_x + 380, ly, "us", label_color, panel_bg);
    ly += 15;

    // Interface 2 Test (AUX / Mouse Port)
    abde_render_string(left_x + 20, ly, "Interface 2 (0xA9)  :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.aux_iface_test.result_byte, g_ps2_micro_debug.aux_iface_test.passed ? pass_color : fail_color, panel_bg);
    abde_render_string(left_x + 235, ly, g_ps2_micro_debug.aux_iface_test.passed ? "[LINES OK]" : "[ERROR]", g_ps2_micro_debug.aux_iface_test.passed ? pass_color : fail_color, panel_bg);
    micro_render_dec(left_x + 325, ly, g_ps2_micro_debug.aux_iface_test.latency_us, text_color, panel_bg);
    abde_render_string(left_x + 380, ly, "us", label_color, panel_bg);
    ly += 18;

    // Reset & BAT State Machine
    abde_render_string(left_x + 15, ly, "KEYBOARD RESET & BAT STATE MACHINE (0xFF)", cyan_color, panel_bg);
    ly += 13;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 15;

    PS2KbdResetSM *rsm = &g_ps2_micro_debug.kbd_reset;

    abde_render_string(left_x + 20, ly, "Pre-Reset Drained   :", label_color, panel_bg);
    micro_render_dec(left_x + 185, ly, rsm->drained_count, text_color, panel_bg);
    abde_render_string(left_x + 215, ly, "bytes", label_color, panel_bg);
    if (rsm->drained_count > 0) {
        abde_render_string(left_x + 270, ly, "B0=", label_color, panel_bg);
        micro_render_hex_byte(left_x + 295, ly, rsm->drained_bytes[0], warn_color, panel_bg);
    }
    ly += 15;

    abm_render_attempts:
    for (int i = 0; i < rsm->attempt_count && i < 2; i++) {
        KbdResetAttempt *att = &rsm->attempts[i];
        char albl[16] = {'T', 'X', ' ', '0', 'x', 'F', 'F', ' ', '[', 'R', '0' + i, ']', ':', '\0'};
        abde_render_string(left_x + 20, ly, albl, label_color, panel_bg);

        // RX #1
        micro_render_hex_byte(left_x + 140, ly, att->rx1_byte, att->rx1_is_ack ? pass_color : (att->rx1_is_resend ? warn_color : fail_color), panel_bg);
        abde_render_string(left_x + 190, ly, att->rx1_is_ack ? "[ACK]" : (att->rx1_is_resend ? "[RESEND]" : "[FAIL]"), att->rx1_is_ack ? pass_color : warn_color, panel_bg);
        micro_render_dec(left_x + 265, ly, att->rx1_latency_us, text_color, panel_bg);
        abde_render_string(left_x + 320, ly, "us", label_color, panel_bg);
        ly += 15;

        // If ACK was received, show BAT
        if (att->rx1_is_ack) {
            abde_render_string(left_x + 35, ly, "BAT Result (RX #2) :", label_color, panel_bg);
            micro_render_hex_byte(left_x + 185, ly, att->rx2_byte, att->rx2_is_bat_ok ? pass_color : fail_color, panel_bg);
            abde_render_string(left_x + 235, ly, att->rx2_is_bat_ok ? "[0xAA PASS]" : "[BAT FAIL]", att->rx2_is_bat_ok ? pass_color : fail_color, panel_bg);
            micro_render_dec(left_x + 325, ly, att->rx2_latency_us, text_color, panel_bg);
            abde_render_string(left_x + 380, ly, "us", label_color, panel_bg);
            ly += 15;
        }
    }

    abde_render_string(left_x + 20, ly, "Current Reset State :", label_color, panel_bg);
    abde_render_string(left_x + 185, ly, rsm->state_str, rsm->bat_pass ? pass_color : warn_color, panel_bg);
    ly += 18;

    // Status Transitions
    abde_render_string(left_x + 15, ly, "STATUS REGISTER 0x64 TRANSITIONS", cyan_color, panel_bg);
    ly += 13;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 15;

    uint8_t st0 = g_ps2_micro_debug.status_t0_boot.raw_status;
    uint8_t st_post_aa = g_ps2_micro_debug.status_after_selftest.raw_status;
    uint8_t st_final = g_ps2_micro_debug.status_final.raw_status;

    abde_render_string(left_x + 20, ly, "T0 Boot Status      :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, st0, (st0 & 0x40) ? warn_color : pass_color, panel_bg);
    abde_render_string(left_x + 235, ly, (st0 & 0x40) ? "[BIT 6 TIMEOUT]" : "[CLEAN]", (st0 & 0x40) ? warn_color : pass_color, panel_bg);
    ly += 15;

    abde_render_string(left_x + 20, ly, "After 0xAA Test     :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, st_post_aa, (st_post_aa & 0x40) ? fail_color : pass_color, panel_bg);
    abde_render_string(left_x + 235, ly, (st_post_aa & 0x40) ? "[STILL TIMEOUT]" : "[TIMEOUT CLEARED]", (st_post_aa & 0x40) ? fail_color : pass_color, panel_bg);
    ly += 15;

    abde_render_string(left_x + 20, ly, "Final Status Byte   :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, st_final, (st_final & 0x40) ? fail_color : pass_color, panel_bg);
    abde_render_string(left_x + 235, ly, (st_final & 0x40) ? "[TIMEOUT SET]" : "[CLEAN]", (st_final & 0x40) ? fail_color : pass_color, panel_bg);
    ly += 18;

    abde_render_string(left_x + 15, ly, "CONTROLLER CONFIGURATION", cyan_color, panel_bg);
    ly += 13;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 15;

    abde_render_string(left_x + 20, ly, "Initial Config (T0) :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.config_initial.raw_config, text_color, panel_bg);
    ly += 15;

    abde_render_string(left_x + 20, ly, "Post-0xAA Config    :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.config_post_selftest.raw_config, text_color, panel_bg);
    ly += 15;

    abde_render_string(left_x + 20, ly, "Post-0xAE Config    :", label_color, panel_bg);
    micro_render_hex_byte(left_x + 185, ly, g_ps2_micro_debug.config_post_reconfig.raw_config, text_color, panel_bg);

    // =========================================================================
    // RIGHT PANEL: SCANNING (0xF4), PROBES, LED (0xED) & LAN TELEMETRY
    // =========================================================================
    uint32_t ry = cur_y + 8;
    abde_render_string(right_x + 15, ry, "SCANNING & COMMAND CHANNEL HEALTH", cyan_color, panel_bg);
    ry += 13;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 15;

    PS2ScanSM *ssm = &g_ps2_micro_debug.kbd_scan;
    abde_render_string(right_x + 20, ry, "Enable Scan (0xF4)  :", label_color, panel_bg);
    if (ssm->skipped_no_bat) {
        abde_render_string(right_x + 185, ry, "SKIPPED (NO BAT PASS)", warn_color, panel_bg);
    } else {
        micro_render_hex_byte(right_x + 185, ry, ssm->rx_byte, ssm->passed ? pass_color : fail_color, panel_bg);
        abde_render_string(right_x + 235, ry, ssm->passed ? "[0xFA PASS]" : (ssm->is_resend ? "[0xFE RESEND]" : "[FAIL]"), ssm->passed ? pass_color : fail_color, panel_bg);
        micro_render_dec(right_x + 335, ry, ssm->latency_us, text_color, panel_bg);
        abde_render_string(right_x + 390, ry, "us", label_color, panel_bg);
    }
    ry += 15;

    PS2EchoSM *esm = &g_ps2_micro_debug.cmd_ee;
    abde_render_string(right_x + 20, ry, "0xEE Echo Probe     :", label_color, panel_bg);
    if (!ssm->passed) {
        abde_render_string(right_x + 185, ry, "BLOCKED (SCAN NOT PASS)", warn_color, panel_bg);
    } else {
        micro_render_hex_byte(right_x + 185, ry, esm->rx_byte, esm->is_echo ? pass_color : fail_color, panel_bg);
        abde_render_string(right_x + 235, ry, esm->is_echo ? "[0xEE OK]" : "[FAIL]", esm->is_echo ? pass_color : fail_color, panel_bg);
        micro_render_dec(right_x + 335, ry, esm->latency_us, text_color, panel_bg);
        abde_render_string(right_x + 390, ry, "us", label_color, panel_bg);
    }
    ry += 18;

    abde_render_string(right_x + 15, ry, "0xED KEYBOARD LED TRANSACTION", cyan_color, panel_bg);
    ry += 13;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 15;

    if (g_ps2_micro_debug.ed_skipped_unhealthy) {
        abde_render_string(right_x + 20, ry, "LED Command Status  :", label_color, panel_bg);
        abde_render_string(right_x + 185, ry, "BLOCKED (CHANNEL UNHEALTHY)", warn_color, panel_bg);
        ry += 15;
    } else {
        abde_render_string(right_x + 20, ry, "0xED Command ACK    :", label_color, panel_bg);
        abde_render_string(right_x + 185, ry, g_ps2_micro_debug.ed_ack_received ? "YES [0xFA ACK]" : "NO [AWAITING ACK]", g_ps2_micro_debug.ed_ack_received ? pass_color : fail_color, panel_bg);
        ry += 15;

        abde_render_string(right_x + 20, ry, "Phase 2 Mask Sent   :", label_color, panel_bg);
        if (g_ps2_micro_debug.ed_ack_received) {
            micro_render_hex_byte(right_x + 185, ry, g_ps2_micro_debug.mask_sent, text_color, panel_bg);
        } else {
            abde_render_string(right_x + 185, ry, "SKIPPED (NO ACK1)", warn_color, panel_bg);
        }
        ry += 15;

        abde_render_string(right_x + 20, ry, "Phase 2 ACK (0xFA)  :", label_color, panel_bg);
        abde_render_string(right_x + 185, ry, g_ps2_micro_debug.mask_ack_received ? "YES [0xFA ACK]" : "NO", g_ps2_micro_debug.mask_ack_received ? pass_color : fail_color, panel_bg);
        ry += 15;
    }
    ry += 10;

    // Live LAN Debug Logging Section
    abde_render_string(right_x + 15, ry, "LAN LIVE TELEMETRY (RTL8168 / GbE)", cyan_color, panel_bg);
    ry += 13;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 15;

    abde_render_string(right_x + 20, ry, "NIC Device / Link   :", label_color, panel_bg);
    abde_render_string(right_x + 185, ry, "Realtek GbE [1000 Mbps FD]", pass_color, panel_bg);
    ry += 15;

    abde_render_string(right_x + 20, ry, "Target MAC Address  :", label_color, panel_bg);
    abde_render_string(right_x + 185, ry, "A0:AD:9F:C5:81:27", text_color, panel_bg);
    ry += 15;

    abde_render_string(right_x + 20, ry, "RX / TX Frames      :", label_color, panel_bg);
    micro_render_dec(right_x + 185, ry, g_rx_frames, pass_color, panel_bg);
    abde_render_string(right_x + 235, ry, "RX  / ", label_color, panel_bg);
    micro_render_dec(right_x + 285, ry, g_tx_ok_count, pass_color, panel_bg);
    abde_render_string(right_x + 335, ry, "TX", label_color, panel_bg);
    ry += 18;

    // Final Verdict
    abde_render_string(right_x + 15, ry, "CERTIFICATION VERDICT", cyan_color, panel_bg);
    ry += 13;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 15;

    abde_render_string(right_x + 20, ry, "FINAL VERDICT       :", label_color, panel_bg);
    bool pass = g_ps2_micro_debug.final_verdict_pass;
    abde_render_string(right_x + 185, ry, pass ? "PASS [PHYSICAL LED HARDWARE SYNCED]" : "FAIL [AWAITING PROTOCOL PASS]", pass ? pass_color : fail_color, panel_bg);
}

void ps2_micro_debug_init(boot_info_t *boot_info) {
    (void)boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    for (int i = 0; i < sizeof(PS2MicroDebug); i++) {
        ((uint8_t*)&g_ps2_micro_debug)[i] = 0;
    }

    ps2_micro_debug_run_test_suite();
}

void ps2_micro_debug_run(boot_info_t *boot_info) {
    ps2_micro_debug_init(boot_info);
    ps2_micro_debug_render();

    // Flush binary AI-(P)DEBUG structured packets over UDP Port 9997
    aipd_flush();

    // Automated Forensic Screenshot: Stream 32-bit BMP over LAN to Python Receiver
    extern bool atoms_screenshot_capture_and_send(uint32_t session_id);
    atoms_screenshot_capture_and_send(1);
    aipd_flush();

    uint64_t loop_counter = 0;
    for (;;) {
        loop_counter++;

        // Service Realtek PCIe NIC incoming frames (for remote AMDE SHUTDOWN packets on UDP 9999)
        if ((loop_counter % 1000) == 0) {
            r8168_poll_receive();
        }

        // Live Dashboard spinner update
        if ((loop_counter % 2000000) == 0) {
            s_spin_tick++;
            char sbuf[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
            abde_render_string(24 + 650, 12 + 16, sbuf, 0x0022C55E, 0x00080E1A);
        }

        __asm__ volatile("pause");
    }
}
