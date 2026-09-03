#include "keyboard_led_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "drivers/input/ps2/ps2.h"
#include "arch/x86_64/io/port_io.h"

extern void com1_puts(const char *s);
extern bool keyboard_get_caps_lock(void);
extern bool keyboard_get_num_lock(void);
extern bool keyboard_get_scroll_lock(void);
extern uint8_t keyboard_get_led_mask(void);
extern bool ps2_keyboard_set_leds(uint8_t led_mask);
extern bool ps2_keyboard_trace_transaction(uint8_t led_mask, PS2RawTrace *trace);
extern void ps2_run_controller_diag(PS2ControllerDiag *diag);

static KeyboardLEDDebugStats g_kb_debug_stats;
static volatile uint64_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

static void com1_put_hex_byte(uint8_t b) {
    char buf[5];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    buf[2] = hex[(b >> 4) & 0xF];
    buf[3] = hex[b & 0xF];
    buf[4] = '\0';
    com1_puts(buf);
}

static void com1_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }
    com1_puts(&buf[pos + 1]);
}

static void kb_dbg_render_hex_byte(uint32_t x, uint32_t y, uint8_t b, uint32_t color, uint32_t bg) {
    char buf[5];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    buf[2] = hex[(b >> 4) & 0xF];
    buf[3] = hex[b & 0xF];
    buf[4] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void kb_dbg_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
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

void keyboard_led_debug_render(void) {
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
    uint32_t start_y = 16;

    // Header Banner
    abde_render_string(start_x, start_y,      "==========================================================================================", cyan_color, bg_color);
    abde_render_string(start_x + 160, start_y + 18, "ATOMS OS - PS/2 KEYBOARD & LED ROOT-CAUSE AUDIT", title_color, bg_color);
    char spin_str[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
    abde_render_string(start_x + 610, start_y + 18, spin_str, pass_color, bg_color);
    abde_render_string(start_x + 640, start_y + 18, "DIAGNOSTIC AUDIT MODE", warn_color, bg_color);
    abde_render_string(start_x, start_y + 36, "==========================================================================================", cyan_color, bg_color);

    uint32_t cur_y = start_y + 54;
    uint32_t left_x = start_x;
    uint32_t right_x = start_x + 470;
    uint32_t panel_w = 450;
    uint32_t panel_h = 670;

    abde_fill_rect(left_x, cur_y, panel_w, panel_h, panel_bg);
    abde_fill_rect(right_x, cur_y, panel_w, panel_h, panel_bg);

    // =========================================================================
    // LEFT PANEL: 8042 CONTROLLER STATE & BASELINE COMMAND TESTS
    // =========================================================================
    uint32_t ly = cur_y + 12;

    // 1. 8042 CONTROLLER CONFIG
    abde_render_string(left_x + 15, ly, "8042 CONTROLLER CONFIG (CMD 0x20)", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Config Byte (0x20):", label_color, panel_bg);
    kb_dbg_render_hex_byte(left_x + 175, ly, g_kb_debug_stats.controller_diag.controller_config_byte, text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Translation (Bit6):", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, g_kb_debug_stats.controller_diag.translation_enabled ? "ENABLED (Set2->Set1)" : "DISABLED (Native)", g_kb_debug_stats.controller_diag.translation_enabled ? pass_color : warn_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Kbd Clock (Bit4)  :", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, g_kb_debug_stats.controller_diag.kbd_clock_enabled ? "ENABLED [UNINHIBITED]" : "DISABLED [INHIBITED]", g_kb_debug_stats.controller_diag.kbd_clock_enabled ? pass_color : fail_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Mouse Clock (Bit5):", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, g_kb_debug_stats.controller_diag.mouse_clock_enabled ? "ENABLED [UNINHIBITED]" : "DISABLED", text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "IRQ1 / IRQ12 State:", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, (g_kb_debug_stats.controller_diag.irq1_enabled && g_kb_debug_stats.controller_diag.irq12_enabled) ? "IRQ1=1 IRQ12=1 [OK]" : "MASKED IN CONTROLLER", pass_color, panel_bg);
    ly += 24;

    // 2. BASELINE COMMAND AUDIT
    abde_render_string(left_x + 15, ly, "BASELINE KEYBOARD COMMAND AUDIT", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Command 0xEE (Echo):", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.echo_rx_valid) {
        kb_dbg_render_hex_byte(left_x + 175, ly, g_kb_debug_stats.controller_diag.echo_rx_byte, pass_color, panel_bg);
        abde_render_string(left_x + 220, ly, "[ECHO OK]", pass_color, panel_bg);
    } else {
        abde_render_string(left_x + 175, ly, "[TIMEOUT / NO ECHO]", fail_color, panel_bg);
    }
    ly += 18;
    abde_render_string(left_x + 20, ly, "Command 0xF4 (Scan):", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.f4_rx_valid) {
        kb_dbg_render_hex_byte(left_x + 175, ly, g_kb_debug_stats.controller_diag.f4_rx_byte, g_kb_debug_stats.controller_diag.f4_ack_received ? pass_color : fail_color, panel_bg);
        abde_render_string(left_x + 220, ly, g_kb_debug_stats.controller_diag.f4_ack_received ? "[ACK=0xFA OK]" : "[NON-ACK BYTE]", g_kb_debug_stats.controller_diag.f4_ack_received ? pass_color : fail_color, panel_bg);
    } else {
        abde_render_string(left_x + 175, ly, "[TIMEOUT / NO ACK]", fail_color, panel_bg);
    }
    ly += 18;
    abde_render_string(left_x + 20, ly, "Command 0xED (LED) :", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.ed_trace.rx_byte1_valid) {
        kb_dbg_render_hex_byte(left_x + 175, ly, g_kb_debug_stats.controller_diag.ed_trace.rx_byte1, g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? pass_color : fail_color, panel_bg);
        abde_render_string(left_x + 220, ly, g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? "[ACK=0xFA OK]" : "[NON-ACK BYTE]", g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? pass_color : fail_color, panel_bg);
    } else {
        abde_render_string(left_x + 175, ly, "[TIMEOUT / NO ACK]", fail_color, panel_bg);
    }
    ly += 24;

    // 3. LOGICAL LOCK STATES
    abde_render_string(left_x + 15, ly, "LOGICAL LOCK STATE & LED MASK", cyan_color, panel_bg);
    ly += 16;
    abde_render_string(left_x + 15, ly, "---------------------------------------------", label_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Caps Lock State   :", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, g_kb_debug_stats.caps_lock ? "ON  [ACTIVE]" : "OFF [IDLE]", g_kb_debug_stats.caps_lock ? pass_color : text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Num Lock State    :", label_color, panel_bg);
    abde_render_string(left_x + 175, ly, g_kb_debug_stats.num_lock ? "ON  [ACTIVE]" : "OFF [IDLE]", g_kb_debug_stats.num_lock ? pass_color : text_color, panel_bg);
    ly += 18;
    abde_render_string(left_x + 20, ly, "Current LED Mask  :", label_color, panel_bg);
    char mask_str[8] = {'0', 'x', '0', '0' + (g_kb_debug_stats.current_led_mask & 0x7), ' ', '(', '\0'};
    abde_render_string(left_x + 175, ly, mask_str, pass_color, panel_bg);
    if (g_kb_debug_stats.current_led_mask == 0) abde_render_string(left_x + 220, ly, "None)", label_color, panel_bg);
    else if (g_kb_debug_stats.current_led_mask == 2) abde_render_string(left_x + 220, ly, "Num)", pass_color, panel_bg);
    else if (g_kb_debug_stats.current_led_mask == 4) abde_render_string(left_x + 220, ly, "Caps)", pass_color, panel_bg);
    else if (g_kb_debug_stats.current_led_mask == 6) abde_render_string(left_x + 220, ly, "Num+Caps)", pass_color, panel_bg);
    else abde_render_string(left_x + 220, ly, "All)", pass_color, panel_bg);

    // =========================================================================
    // RIGHT PANEL: RAW 0xED TRANSACTION TRACE & VERDICTS
    // =========================================================================
    uint32_t ry = cur_y + 12;

    abde_render_string(right_x + 15, ry, "RAW SINGLE 0xED TRANSACTION TRACE", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "IRQ Mask State    :", label_color, panel_bg);
    abde_render_string(right_x + 175, ry, "irq_save() [MASKED]", pass_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Status Before Drain:", label_color, panel_bg);
    kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.status_before_drain, text_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Status Before 0xED :", label_color, panel_bg);
    kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.status_before_ed, text_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "Status After Write :", label_color, panel_bg);
    kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.status_after_ed_write, text_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "AUX Mouse Encount 1:", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.ed_trace.aux_mouse_encountered1) {
        abde_render_string(right_x + 175, ry, "YES [PRESERVED]", pass_color, panel_bg);
    } else {
        abde_render_string(right_x + 175, ry, "NONE (Direct)", text_color, panel_bg);
    }
    ry += 18;
    abde_render_string(right_x + 20, ry, "KBD RX Byte 1 (FA):", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.ed_trace.rx_byte1_valid) {
        kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.rx_byte1, g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? pass_color : fail_color, panel_bg);
        abde_render_string(right_x + 220, ry, g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? "[ACK=0xFA YES]" : "[NON-ACK]", g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? pass_color : fail_color, panel_bg);
    } else {
        abde_render_string(right_x + 175, ry, "[NO KBD BYTE / TIMEOUT]", fail_color, panel_bg);
    }
    ry += 18;
    abde_render_string(right_x + 20, ry, "TX LED Mask Sent   :", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa) {
        kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.mask_sent, text_color, panel_bg);
    } else {
        abde_render_string(right_x + 175, ry, "NOT SENT (NO ACK1)", warn_color, panel_bg);
    }
    ry += 18;
    abde_render_string(right_x + 20, ry, "KBD RX Byte 2 (FA):", label_color, panel_bg);
    if (g_kb_debug_stats.controller_diag.ed_trace.rx_byte2_valid) {
        kb_dbg_render_hex_byte(right_x + 175, ry, g_kb_debug_stats.controller_diag.ed_trace.rx_byte2, g_kb_debug_stats.controller_diag.ed_trace.ack2_is_fa ? pass_color : fail_color, panel_bg);
        abde_render_string(right_x + 220, ry, g_kb_debug_stats.controller_diag.ed_trace.ack2_is_fa ? "[ACK=0xFA YES]" : "[NON-ACK]", g_kb_debug_stats.controller_diag.ed_trace.ack2_is_fa ? pass_color : fail_color, panel_bg);
    } else {
        abde_render_string(right_x + 175, ry, "[NO KBD BYTE / TIMEOUT]", fail_color, panel_bg);
    }
    ry += 24;

    // 6. FORENSIC CERTIFICATION VERDICT
    abde_render_string(right_x + 15, ry, "HARDWARE FIX CERTIFICATION VERDICT", cyan_color, panel_bg);
    ry += 16;
    abde_render_string(right_x + 15, ry, "---------------------------------------------", label_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "HARDWARE PROTOCOL :", label_color, panel_bg);
    abde_render_string(right_x + 175, ry, "0xED -> 0xFA -> MASK -> 0xFA", text_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "PHYSICAL LED SYNC :", label_color, panel_bg);
    bool overall_pass = g_kb_debug_stats.controller_diag.ed_trace.final_transaction_pass;
    abde_render_string(right_x + 175, ry, overall_pass ? "ACTIVE & SYNCHRONIZED [PASS]" : "DESYNCHRONIZED [FAIL]", overall_pass ? pass_color : fail_color, panel_bg);
    ry += 18;
    abde_render_string(right_x + 20, ry, "FINAL VERDICT     :", label_color, panel_bg);
    abde_render_string(right_x + 175, ry, overall_pass ? "PASS [CASE B HARDWARE FIX CERTIFIED]" : "FAIL [AWAITING HARDWARE ACK]", overall_pass ? pass_color : fail_color, panel_bg);
}

void keyboard_led_debug_init(boot_info_t *boot_info) {
    (void)boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    g_kb_debug_stats.status_port        = 0x64;
    g_kb_debug_stats.data_port          = 0x60;
    g_kb_debug_stats.irq_number         = 1;
    g_kb_debug_stats.controller_ready   = true;
    g_kb_debug_stats.caps_lock          = keyboard_get_caps_lock();
    g_kb_debug_stats.num_lock           = keyboard_get_num_lock();
    g_kb_debug_stats.scroll_lock        = keyboard_get_scroll_lock();
    g_kb_debug_stats.current_led_mask   = keyboard_get_led_mask();

    g_kb_debug_stats.commands_sent      = 0;
    g_kb_debug_stats.acks_received      = 0;
    g_kb_debug_stats.resends_received   = 0;
    g_kb_debug_stats.timeouts_occurred  = 0;

    com1_puts("\r\n========================================================\r\n");
    com1_puts("[KB_LED] PS/2 CONTROLLER & KEYBOARD ROOT-CAUSE AUDIT v4\r\n");
    com1_puts("[KB_LED] HARDWARE STATUS PORT = 0x64, DATA PORT = 0x60\r\n\r\n");

    // Execute Controller and Baseline Command Diagnostics
    ps2_run_controller_diag(&g_kb_debug_stats.controller_diag);

    com1_puts("[KB_DIAG] CONTROLLER CONFIG BYTE (0x20): ");
    com1_put_hex_byte(g_kb_debug_stats.controller_diag.controller_config_byte);
    com1_puts(" [TRANSLATION: ");
    com1_puts(g_kb_debug_stats.controller_diag.translation_enabled ? "ENABLED" : "DISABLED");
    com1_puts(", KBD_CLK: ");
    com1_puts(g_kb_debug_stats.controller_diag.kbd_clock_enabled ? "ENABLED" : "DISABLED");
    com1_puts(", MOUSE_CLK: ");
    com1_puts(g_kb_debug_stats.controller_diag.mouse_clock_enabled ? "ENABLED" : "DISABLED");
    com1_puts("]\r\n");

    com1_puts("[KB_DIAG] CMD 0xEE (ECHO) TEST: ");
    if (g_kb_debug_stats.controller_diag.echo_rx_valid) {
        com1_puts("RX="); com1_put_hex_byte(g_kb_debug_stats.controller_diag.echo_rx_byte);
        com1_puts(" [PASS]\r\n");
    } else {
        com1_puts("[TIMEOUT / NO RESPONSE]\r\n");
    }

    com1_puts("[KB_DIAG] CMD 0xF4 (SCAN) TEST: ");
    if (g_kb_debug_stats.controller_diag.f4_rx_valid) {
        com1_puts("RX="); com1_put_hex_byte(g_kb_debug_stats.controller_diag.f4_rx_byte);
        com1_puts(g_kb_debug_stats.controller_diag.f4_ack_received ? " [ACK 0xFA PASS]\r\n" : " [NON-ACK]\r\n");
    } else {
        com1_puts("[TIMEOUT / NO RESPONSE]\r\n");
    }

    com1_puts("[KB_DIAG] CMD 0xED (LED) SINGLE TEST:\r\n");
    com1_puts("  STATUS BEFORE 0xED : "); com1_put_hex_byte(g_kb_debug_stats.controller_diag.ed_trace.status_before_ed);
    com1_puts("\r\n  STATUS AFTER WRITE : "); com1_put_hex_byte(g_kb_debug_stats.controller_diag.ed_trace.status_after_ed_write);
    com1_puts("\r\n  AUX MOUSE ENCOUNTER: "); com1_puts(g_kb_debug_stats.controller_diag.ed_trace.aux_mouse_encountered1 ? "YES [PRESERVED]" : "NO");
    com1_puts("\r\n  KBD RX BYTE 1 (ACK): ");
    if (g_kb_debug_stats.controller_diag.ed_trace.rx_byte1_valid) {
        com1_put_hex_byte(g_kb_debug_stats.controller_diag.ed_trace.rx_byte1);
        com1_puts(g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa ? " [ACK=0xFA PASS]\r\n" : " [NON-ACK]\r\n");
    } else {
        com1_puts("[NO KBD BYTE / TIMEOUT]\r\n");
    }

    if (g_kb_debug_stats.controller_diag.ed_trace.ack1_is_fa) {
        com1_puts("  TX LED MASK        : "); com1_put_hex_byte(g_kb_debug_stats.controller_diag.ed_trace.mask_sent);
        com1_puts("\r\n  KBD RX BYTE 2 (ACK): ");
        if (g_kb_debug_stats.controller_diag.ed_trace.rx_byte2_valid) {
            com1_put_hex_byte(g_kb_debug_stats.controller_diag.ed_trace.rx_byte2);
            com1_puts(g_kb_debug_stats.controller_diag.ed_trace.ack2_is_fa ? " [ACK=0xFA PASS]\r\n" : " [NON-ACK]\r\n");
        } else {
            com1_puts("[NO KBD BYTE / TIMEOUT]\r\n");
        }
    } else {
        com1_puts("  TX LED MASK        : [SKIPPED - NO ACK1]\r\n");
    }

    com1_puts("[KB_DIAG] FINAL RESULT = ");
    com1_puts(g_kb_debug_stats.controller_diag.ed_trace.final_transaction_pass ? "PASS" : "FAIL");
    com1_puts("\r\n========================================================\r\n\r\n");
}

void keyboard_led_debug_run(boot_info_t *boot_info) {
    keyboard_led_debug_init(boot_info);
    keyboard_led_debug_render();

    extern bool r8168_poll_receive(void);

    uint64_t loop_counter = 0;
    for (;;) {
        loop_counter++;

        // Service Realtek PCIe NIC incoming frames (for remote AMDE SHUTDOWN / REBOOT packets on UDP 9999)
        if ((loop_counter % 1000) == 0) {
            r8168_poll_receive();
        }

        if ((loop_counter % 2000000) == 0) {
            s_spin_tick++;
            g_kb_debug_stats.caps_lock = keyboard_get_caps_lock();
            g_kb_debug_stats.num_lock = keyboard_get_num_lock();
            g_kb_debug_stats.scroll_lock = keyboard_get_scroll_lock();
            g_kb_debug_stats.current_led_mask = keyboard_get_led_mask();

            char sbuf[4] = {'[', s_spin_chars[s_spin_tick % 4], ']', '\0'};
            abde_render_string(24 + 610, 16 + 18, sbuf, 0x0022C55E, 0x00080E1A);
        }
        __asm__ volatile("pause");
    }
}
