#include "cursor_certification.h"
#include "kernel/drivers/video/vbe/vbe.h"
#include "bovisual/Include/core.h"
#include "bovisual/Include/graphics.h"
#include "bovisual/Include/drawing.h"
#include "bovisual/Include/text.h"
#include "kernel/drivers/input/input.h"
#include "drivers/input/ps2/mouse.h"
#include "kernel/drivers/input/core/input_core.h"
#include "kernel/drivers/input/pointer/pointer_state.h"
#include "kernel/drivers/input/pointer/pointer_bounds.h"
#include "kernel/drivers/input/cursor/cursor_state.h"
#include "kernel/drivers/input/cursor/cursor_engine.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/ahme/include/ahme.h"
#include "kernel/drivers/net/r8168/r8168.h"
#include <stdint.h>
#include <stdbool.h>

extern void com1_puts(const char *s);
extern volatile uint64_t g_irq12_count;
extern volatile uint64_t g_pointer_event_count;

static boot_info_t *g_cert_boot_info = NULL;

static void u64_to_str(uint64_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[32]; int i = 0;
    while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

static void u64_to_hex_str(uint64_t val, char *buf) {
    char hex[] = "0123456789ABCDEF";
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[32]; int i = 0;
    while (val > 0) { tmp[i++] = hex[val & 0xF]; val >>= 4; }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

static void i32_to_str(int32_t val, char *buf) {
    if (val < 0) {
        buf[0] = '-';
        u64_to_str((uint64_t)(-val), buf + 1);
    } else {
        u64_to_str((uint64_t)val, buf);
    }
}

static void str_cat(char *dst, const char *src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

void atoms_cursor_certification_init(boot_info_t *boot_info) {
    g_cert_boot_info = boot_info;
}

void atoms_cursor_certification_task(void) {
    com1_puts("\r\n====================================================\r\n");
    com1_puts("   ATOMS OS — INTEL H81 MOUSE FORENSIC CERTIFICATION\r\n");
    com1_puts("====================================================\r\n");

    if (!g_cert_boot_info) {
        com1_puts("[CERT] ERROR: Boot Info is NULL!\r\n");
        return;
    }

    if (!g_abde.framebuffer && g_cert_boot_info) {
        g_abde.framebuffer = g_cert_boot_info->vbe_framebuffer;
        g_abde.width = g_cert_boot_info->vbe_width;
        g_abde.height = g_cert_boot_info->vbe_height;
        g_abde.pitch = g_cert_boot_info->vbe_pitch;
    }

    com1_puts("[CERT] Initializing Input Engine Pipeline...\r\n");
    kernel_input_init();

    com1_puts("[CERT] Initializing 8042 PS/2 Controller Driver...\r\n");
    ps2_mouse_init();

    cursor_state_set_visible(true);

    com1_puts("[CERT] Entering Phase C Diagnostic Loop...\r\n");

    uint64_t loop_count = 0;

    extern bool g_cursor_cert_ui_active;
    g_cursor_cert_ui_active = true;

    for (;;) {
        loop_count++;

        input_core_dispatch_events();

        const PointerState *pstate = pointer_state_get();
        int32_t cx = pstate ? pstate->current_x : 0;
        int32_t cy = pstate ? pstate->current_y : 0;
        bool left_pressed = pstate ? ((pstate->button_mask & (1 << 0)) != 0) : false;
        bool right_pressed = pstate ? ((pstate->button_mask & (1 << 1)) != 0) : false;

        PS2MouseDiagnostics diag;
        ps2_mouse_get_diagnostics(&diag);

        AHMEState *ahme = ahme_get_state();

        uint32_t bg_color = 0xFF1A252F;
        if (left_pressed) bg_color = 0xFF1E8449;
        else if (right_pressed) bg_color = 0xFF922B21;

        uint32_t panel_w = 780;
        uint32_t panel_h = 760;
        uint32_t panel_x = (g_abde.width > panel_w) ? (g_abde.width - panel_w) / 2 : 10;
        uint32_t panel_y = (g_abde.height > panel_h) ? (g_abde.height - panel_h) / 2 : 10;
        static uint32_t last_bg_color = 0;
        bool need_bg_redraw = (loop_count == 1 || bg_color != last_bg_color || (loop_count % 10 == 0));
        last_bg_color = bg_color;

        if (need_bg_redraw) {
            // Fill entire screen background once on first loop to erase ABDE dashboard
            if (loop_count == 1) {
                abde_fill_rect(0, 0, g_abde.width, g_abde.height, 0xFF0F172A);
            }

            // Draw Panel Background & Border directly to physical VRAM
            abde_fill_rect(panel_x, panel_y, panel_w, panel_h, bg_color);
            abde_fill_rect(panel_x, panel_y, panel_w, 2, 0xFFECF0F1);
            abde_fill_rect(panel_x, panel_y + panel_h - 2, panel_w, 2, 0xFFECF0F1);
            abde_fill_rect(panel_x, panel_y, 2, panel_h, 0xFFECF0F1);
            abde_fill_rect(panel_x + panel_w - 2, panel_y, 2, panel_h, 0xFFECF0F1);

            // Header Title
            abde_render_string(panel_x + 90, panel_y + 15, "ATOMS OS — REAL-TIME HARDWARE & USB DIAGNOSTIC PANEL", 0xFFECF0F1, bg_color);
            abde_render_string(panel_x + 220, panel_y + 35, "LIVE BARE-METAL H81 MILESTONE TELEMETRY", 0xFFF1C40F, bg_color);
            abde_fill_rect(panel_x + 20, panel_y + 55, panel_w - 40, 1, 0xFFBDC3C7);
        }

        #include "kernel/drivers/usb/core/usb_core.h"

        // 9 Real-Time USB Diagnostic Milestones
        abde_render_string(panel_x + 30, panel_y + 65, "1. xHCI Controller Started", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 65, g_usb_diag.xhci_started ? "= PASS" : "= WAIT...", g_usb_diag.xhci_started ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 88, "2. Enable Slot Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 88, g_usb_diag.enable_slot_pass ? "= PASS" : "= WAIT...", g_usb_diag.enable_slot_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 111, "3. Address Device Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 111, g_usb_diag.address_device_pass ? "= PASS" : "= WAIT...", g_usb_diag.address_device_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 134, "4. GET_DESCRIPTOR Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 134, g_usb_diag.get_descriptor_pass ? "= PASS" : "= WAIT...", g_usb_diag.get_descriptor_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 157, "5. SET_CONFIGURATION Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 157, g_usb_diag.set_config_pass ? "= PASS" : "= WAIT...", g_usb_diag.set_config_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 180, "6. Configure Endpoint Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 180, g_usb_diag.configure_ep_pass ? "= PASS" : "= WAIT...", g_usb_diag.configure_ep_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        abde_render_string(panel_x + 30, panel_y + 203, "7. Interrupt IN Transfer Success", 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 203, g_usb_diag.interrupt_in_pass ? "= PASS" : "= WAIT...", g_usb_diag.interrupt_in_pass ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        char s_m_pkt[64] = "8. Mouse Packet Received: "; char b_m_pkt[32]; u64_to_str(g_usb_diag.mouse_packet_count, b_m_pkt); str_cat(s_m_pkt, b_m_pkt);
        abde_render_string(panel_x + 30, panel_y + 226, s_m_pkt, (g_usb_diag.mouse_packet_count > 0) ? 0xFF00FFFF : 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 226, (g_usb_diag.mouse_packet_count > 0) ? "= PASS (RECEIVING)" : "= WAITING INPUT...", (g_usb_diag.mouse_packet_count > 0) ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        char s_k_pkt[64] = "9. Keyboard Packet Received: "; char b_k_pkt[32]; u64_to_str(g_usb_diag.keyboard_packet_count, b_k_pkt); str_cat(s_k_pkt, b_k_pkt);
        abde_render_string(panel_x + 30, panel_y + 249, s_k_pkt, (g_usb_diag.keyboard_packet_count > 0) ? 0xFF00FFFF : 0xFFECF0F1, bg_color);
        abde_render_string(panel_x + 420, panel_y + 249, (g_usb_diag.keyboard_packet_count > 0) ? "= PASS (RECEIVING)" : "= WAITING INPUT...", (g_usb_diag.keyboard_packet_count > 0) ? 0xFF2ECC71 : 0xFFE67E22, bg_color);

        // =====================================================================
        // ATOMS OS PHASE 6 LIVE STALL DETECTOR PANEL & TELEMETRY ENGINE
        // =====================================================================
        extern volatile uint64_t g_irq0_ticks;
        extern volatile uint64_t g_scheduler_ticks;
        extern volatile uint64_t g_heartbeat_ticks;
        extern volatile uint64_t g_xhci_events;
        extern volatile uint64_t g_xhci_transfers;
        extern volatile uint64_t g_usb_hid_packets;
        extern volatile uint64_t g_hida_events;
        extern volatile uint64_t g_mouse_events;
        extern volatile uint64_t g_keyboard_events;

        abde_fill_rect(panel_x + 20, panel_y + 270, panel_w - 40, 1, 0xFFBDC3C7);
        abde_render_string(panel_x + 230, panel_y + 275, "--- ATOMS OS LIVE STALL DETECTOR ---", 0xFFF1C40F, bg_color);

        char s_irq0[32] = "IRQ0 TICKS   : "; char b_irq0[16]; u64_to_str(g_irq0_ticks, b_irq0); str_cat(s_irq0, b_irq0);
        abde_render_string(panel_x + 30, panel_y + 295, s_irq0, 0xFF00FFFF, bg_color);

        char s_sched[32] = "SCHED TICKS  : "; char b_sched[16]; u64_to_str(g_scheduler_ticks, b_sched); str_cat(s_sched, b_sched);
        abde_render_string(panel_x + 280, panel_y + 295, s_sched, 0xFF00FFFF, bg_color);

        char s_hb[32] = "HEARTBEAT    : "; char b_hb[16]; u64_to_str(g_heartbeat_ticks, b_hb); str_cat(s_hb, b_hb);
        abde_render_string(panel_x + 520, panel_y + 295, s_hb, 0xFF00FFFF, bg_color);

        char s_xev[32] = "XHCI EVENTS  : "; char b_xev[16]; u64_to_str(g_xhci_events, b_xev); str_cat(s_xev, b_xev);
        abde_render_string(panel_x + 30, panel_y + 318, s_xev, 0xFFECF0F1, bg_color);

        char s_xtr[32] = "XHCI XFERS   : "; char b_xtr[16]; u64_to_str(g_xhci_transfers, b_xtr); str_cat(s_xtr, b_xtr);
        abde_render_string(panel_x + 280, panel_y + 318, s_xtr, 0xFFECF0F1, bg_color);

        char s_hid[32] = "HID PACKETS  : "; char b_hid[16]; u64_to_str(g_usb_hid_packets, b_hid); str_cat(s_hid, b_hid);
        abde_render_string(panel_x + 520, panel_y + 318, s_hid, 0xFFECF0F1, bg_color);

        char s_hida[32] = "HIDA EVENTS  : "; char b_hida[16]; u64_to_str(g_hida_events, b_hida); str_cat(s_hida, b_hida);
        abde_render_string(panel_x + 30, panel_y + 341, s_hida, 0xFF2ECC71, bg_color);

        char s_mev[32] = "MOUSE EVENTS : "; char b_mev[16]; u64_to_str(g_mouse_events, b_mev); str_cat(s_mev, b_mev);
        abde_render_string(panel_x + 280, panel_y + 341, s_mev, 0xFF2ECC71, bg_color);

        char s_kev[32] = "KBD EVENTS   : "; char b_kev[16]; u64_to_str(g_keyboard_events, b_kev); str_cat(s_kev, b_kev);
        abde_render_string(panel_x + 520, panel_y + 341, s_kev, 0xFF2ECC71, bg_color);

        // Poll Network Hardware RX Ring
        extern bool r8168_poll_receive(void);
        r8168_poll_receive();

        // === LANDBG NIC STATUS (ALWAYS VISIBLE) ===
        extern bool debuglan_active(void);
        extern volatile uint64_t g_debuglan_tx_attempts;
        extern volatile uint64_t g_debuglan_tx_success;
        extern volatile uint64_t g_rx_frames;
        extern volatile uint64_t g_rx_arp_frames;
        extern volatile uint64_t g_rx_ipv4_frames;
        extern volatile uint64_t g_rx_drop_count;
        {
            R8168Device* rdev = r8168_get_device();
            bool active = debuglan_active();
            char nic_line[80] = "LANDBG: ";
            if (active) {
                str_cat(nic_line, "ACTIVE=YES");
            } else {
                str_cat(nic_line, "ACTIVE=NO!");
            }
            str_cat(nic_line, " | R8168=");
            if (rdev) {
                if (rdev->state == R8168_STATE_READY)         str_cat(nic_line, "READY");
                else if (rdev->state == R8168_STATE_PCI_FOUND) str_cat(nic_line, "PCI_ONLY");
                else if (rdev->state == R8168_STATE_FAILED)    str_cat(nic_line, "FAIL");
                else                                           str_cat(nic_line, "UNINIT");
            } else {
                str_cat(nic_line, "NULL");
            }
            uint32_t nic_color = active ? 0xFF2ECC71 : 0xFFE74C3C;
            abde_render_string(panel_x + 30, panel_y + 358, nic_line, nic_color, bg_color);

            // === DEEP HARDWARE FORENSICS REPORT ===
            R8168ForensicReport frep;
            r8168_run_forensics(&frep);

            char h0[128] = "PCI_ID: 0x"; char tmp[32]; u64_to_hex_str(frep.vendor_id, tmp); str_cat(h0, tmp);
            str_cat(h0, ":0x"); u64_to_hex_str(frep.device_id, tmp); str_cat(h0, tmp);
            str_cat(h0, " | REV: 0x"); u64_to_hex_str(frep.revision_id, tmp); str_cat(h0, tmp);
            str_cat(h0, " | SUB: 0x"); u64_to_hex_str(frep.subsystem_vendor_id, tmp); str_cat(h0, tmp);
            str_cat(h0, ":0x"); u64_to_hex_str(frep.subsystem_id, tmp); str_cat(h0, tmp);
            abde_render_string(panel_x + 30, panel_y + 325, h0, 0xFF00FF7F, bg_color);

            char h1[128] = "PCI_CMD: 0x"; u64_to_hex_str(frep.pci_command, tmp); str_cat(h1, tmp);
            str_cat(h1, " (BM:"); str_cat(h1, frep.pci_bus_master ? "YES" : "NO");
            str_cat(h1, "|MEM:"); str_cat(h1, frep.pci_memory_enable ? "YES" : "NO");
            str_cat(h1, "|IO:");  str_cat(h1, frep.pci_io_enable ? "YES" : "NO");
            str_cat(h1, ") | PCI_STAT: 0x"); u64_to_hex_str(frep.pci_status, tmp); str_cat(h1, tmp);
            abde_render_string(panel_x + 30, panel_y + 340, h1, 0xFF3498DB, bg_color);

            char h2[128] = "HW_REG: CMD=0x"; u64_to_hex_str(frep.chip_cmd, tmp); str_cat(h2, tmp);
            str_cat(h2, " | TXCFG=0x"); u64_to_hex_str(frep.tx_config, tmp); str_cat(h2, tmp);
            str_cat(h2, " | CPLUS=0x"); u64_to_hex_str(frep.cplus_cmd, tmp); str_cat(h2, tmp);
            str_cat(h2, " | IMR=0x");   u64_to_hex_str(frep.imr, tmp); str_cat(h2, tmp);
            str_cat(h2, " | ISR=0x");   u64_to_hex_str(frep.isr, tmp); str_cat(h2, tmp);
            abde_render_string(panel_x + 30, panel_y + 355, h2, 0xFFF1C40F, bg_color);

            char h3[128] = "HW_TX_BASE: 0x"; u64_to_hex_str(frep.hw_tx_desc_low, tmp); str_cat(h3, tmp);
            str_cat(h3, " | RING_PA: 0x"); u64_to_hex_str((uint32_t)frep.tx_ring_pa, tmp); str_cat(h3, tmp);
            str_cat(h3, " (MATCH:"); str_cat(h3, frep.tx_ring_base_matches ? "YES" : "NO"); str_cat(h3, ")");
            abde_render_string(panel_x + 30, panel_y + 370, h3, frep.tx_ring_base_matches ? 0xFF2ECC71 : 0xFFE74C3C, bg_color);

            char h4[128] = "TX_HEAD: "; u64_to_str(g_tx_head_snapshot, tmp); str_cat(h4, tmp);
            str_cat(h4, " | TAIL: "); u64_to_str(g_tx_tail_snapshot, tmp); str_cat(h4, tmp);
            str_cat(h4, " | OPTS0=0x"); u64_to_hex_str(frep.desc0_opts1, tmp); str_cat(h4, tmp);
            str_cat(h4, " | DESC0_PA=0x"); u64_to_hex_str(frep.desc0_addr_low, tmp); str_cat(h4, tmp);
            str_cat(h4, " (BUF_MATCH:"); str_cat(h4, frep.desc0_addr_matches ? "YES" : "NO"); str_cat(h4, ")");
            abde_render_string(panel_x + 460, panel_y + 340, h4, 0xFFE67E22, bg_color);

            char h5[128] = "TX_TRY: "; u64_to_str(g_tx_try_count, tmp); str_cat(h5, tmp);
            str_cat(h5, " | TX_OK: "); u64_to_str(g_tx_ok_count, tmp); str_cat(h5, tmp);
            str_cat(h5, " | DROP: "); u64_to_str(g_tx_drop_count, tmp); str_cat(h5, tmp);
            str_cat(h5, " | HW_OWN_CLEAR:"); str_cat(h5, frep.own_cleared_by_hw ? "YES" : "NO");
            abde_render_string(panel_x + 30, panel_y + 385, h5, frep.own_cleared_by_hw ? 0xFF2ECC71 : 0xFFE74C3C, bg_color);

            char rx_line[128] = "RX_FRAMES: "; char b_rxf[16]; u64_to_str(g_rx_frames, b_rxf); str_cat(rx_line, b_rxf);
            str_cat(rx_line, " | ARP: "); char b_arp[16]; u64_to_str(g_rx_arp_frames, b_arp); str_cat(rx_line, b_arp);
            str_cat(rx_line, " | IPV4: "); char b_ip[16]; u64_to_str(g_rx_ipv4_frames, b_ip); str_cat(rx_line, b_ip);
            str_cat(rx_line, " | DROP: "); char b_drp[16]; u64_to_str(g_rx_drop_count, b_drp); str_cat(rx_line, b_drp);
            abde_render_string(panel_x + 460, panel_y + 385, rx_line, 0xFF00FFFF, bg_color);
        }

        // AUTO STALL ANALYZER
        static uint64_t prev_irq0 = 0, prev_sched = 0, prev_hb = 0, prev_xev = 0, prev_hid = 0, prev_hida = 0, prev_mev = 0;
        static uint32_t sample_counter = 0;
        static const char* stall_status = "[SYSTEM INITIALIZING...]";
        static uint32_t stall_color = 0xFFF1C40F;

        sample_counter++;
        if (sample_counter >= 100) { // Every ~1 sec (100 * 10ms)
            if (g_irq0_ticks == prev_irq0) {
                stall_status = "[STALL] INTERRUPTS DEAD";
                stall_color = 0xFFE74C3C;
            } else if (g_scheduler_ticks == prev_sched) {
                stall_status = "[STALL] SCHEDULER DEAD";
                stall_color = 0xFFE74C3C;
            } else if (g_heartbeat_ticks == prev_hb) {
                stall_status = "[STALL] PRIORITY STARVATION";
                stall_color = 0xFFE74C3C;
            } else if (g_usb_diag.xhci_started && g_xhci_events == prev_xev) {
                stall_status = "[STALL] XHCI PIPELINE DEAD";
                stall_color = 0xFFE67E22;
            } else if (g_usb_diag.interrupt_in_pass && g_usb_hid_packets == prev_hid) {
                stall_status = "[STALL] HID DRIVER DEAD";
                stall_color = 0xFFE67E22;
            } else if (g_usb_hid_packets > prev_hid && g_hida_events == prev_hida) {
                stall_status = "[STALL] HIDA ROUTER DROPPING EVENTS";
                stall_color = 0xFFE67E22;
            } else if (g_hida_events > prev_hida && g_mouse_events == prev_mev && g_usb_diag.mouse_packet_count > 0) {
                stall_status = "[STALL] CURSOR PIPELINE DEAD";
                stall_color = 0xFFE67E22;
            } else {
                stall_status = "[SYSTEM OK] PIPELINE HEALTHY";
                stall_color = 0xFF2ECC71;
            }

            prev_irq0 = g_irq0_ticks;
            prev_sched = g_scheduler_ticks;
            prev_hb = g_heartbeat_ticks;
            prev_xev = g_xhci_events;
            prev_hid = g_usb_hid_packets;
            prev_hida = g_hida_events;
            prev_mev = g_mouse_events;
            sample_counter = 0;
        }

        // Update Forensic Engine Ticks
        #include "kernel/drivers/usb/core/usb_forensic_trace.h"
        usb_forensic_update_tick();

        abde_fill_rect(panel_x + 20, panel_y + 395, panel_w - 40, 1, 0xFFBDC3C7);
        abde_render_string(panel_x + 30, panel_y + 400, "USB FORENSIC TRACE PANEL V1 & ROOT CAUSE ENGINE:", 0xFFF1C40F, bg_color);

        uint32_t last_pass = g_usb_forensic.last_successful_stage;
        uint32_t first_fail = g_usb_forensic.first_failed_stage;
        uint32_t progress_pct = (last_pass * 100) / 19;

        char s_prog[64] = "PROGRESS: "; char b_last[16], b_pct[16];
        u64_to_str(last_pass, b_last); u64_to_str(progress_pct, b_pct);
        str_cat(s_prog, b_last); str_cat(s_prog, "/19 STAGES ("); str_cat(s_prog, b_pct); str_cat(s_prog, "% COMPLETE)");
        abde_render_string(panel_x + 440, panel_y + 400, s_prog, g_usb_forensic.is_frozen ? 0xFFE74C3C : 0xFF2ECC71, bg_color);

        char s_last_succ[64] = "LAST SUCCESS : ["; char b_ls[16]; u64_to_str(last_pass, b_ls); str_cat(s_last_succ, b_ls); str_cat(s_last_succ, "] ");
        if (last_pass > 0 && last_pass < USB_STAGE_COUNT) str_cat(s_last_succ, g_usb_forensic.stages[last_pass].name);
        else str_cat(s_last_succ, "NONE");
        abde_render_string(panel_x + 30, panel_y + 423, s_last_succ, 0xFF2ECC71, bg_color);

        char s_first_fail[64] = "FIRST FAILURE: ["; char b_ff[16]; u64_to_str(first_fail, b_ff); str_cat(s_first_fail, b_ff); str_cat(s_first_fail, "] ");
        if (first_fail > 0 && first_fail < USB_STAGE_COUNT) str_cat(s_first_fail, g_usb_forensic.stages[first_fail].name);
        else str_cat(s_first_fail, "NONE");
        abde_render_string(panel_x + 30, panel_y + 443, s_first_fail, g_usb_forensic.is_frozen ? 0xFFE74C3C : 0xFF00FFFF, bg_color);

        char s_subsys[64] = "SUSPECT SUBSYS: "; str_cat(s_subsys, g_usb_forensic.suspect_subsystem);
        abde_render_string(panel_x + 30, panel_y + 463, s_subsys, 0xFFF1C40F, bg_color);

        char s_src[80] = "SOURCE FILE   : "; str_cat(s_src, g_usb_forensic.source_file);
        abde_render_string(panel_x + 30, panel_y + 483, s_src, 0xFFECF0F1, bg_color);

        char s_cause[80] = "ESTIMATED CAUSE: "; str_cat(s_cause, g_usb_forensic.root_cause_desc);
        abde_render_string(panel_x + 30, panel_y + 503, s_cause, g_usb_forensic.is_frozen ? 0xFFE74C3C : 0xFF2ECC71, bg_color);

        // =====================================================================
        // ATOMS OS USB FORENSIC PHASE-3 TELEMETRY PANEL
        // =====================================================================
        #include "kernel/drivers/usb/core/usb_forensic_phase3.h"
        extern volatile const char* g_cfg_failing_req_name;

        // =====================================================================
        // ATOMS OS USB FORENSIC PHASE-4 DMA & MEMORY INTEGRITY PANEL
        // =====================================================================
        abde_fill_rect(panel_x + 20, panel_y + 525, panel_w - 40, 1, 0xFFBDC3C7);
        abde_render_string(panel_x + 30, panel_y + 530, "--- USB FORENSIC PHASE-4 DMA & MEMORY INTEGRITY AUDIT ---", 0xFFF1C40F, bg_color);

        char s_p4_1[128] = "DMA_PHYS: 0x"; char b_dp[32]; u64_to_hex_str(g_usb_phase3.dma_phys, b_dp); str_cat(s_p4_1, b_dp);
        str_cat(s_p4_1, " | DMA_VIRT: 0x"); char b_dv[32]; u64_to_hex_str(g_usb_phase3.dma_virt, b_dv); str_cat(s_p4_1, b_dv);
        abde_render_string(panel_x + 30, panel_y + 550, s_p4_1, 0xFF00FFFF, bg_color);

        char s_p4_2[128] = "USER_BUF: 0x"; char b_ub[32]; u64_to_hex_str(g_usb_phase3.user_buf_virt, b_ub); str_cat(s_p4_2, b_ub);
        str_cat(s_p4_2, " | CFG_BUF: 0x"); char b_cb[32]; u64_to_hex_str(g_usb_phase3.cfg_buf_virt, b_cb); str_cat(s_p4_2, b_cb);
        bool match = (g_usb_phase3.user_buf_virt == g_usb_phase3.cfg_buf_virt);
        str_cat(s_p4_2, match ? " [PTR_MATCH]" : " [BUFFER_MISMATCH_DETECTED]");
        abde_render_string(panel_x + 30, panel_y + 570, s_p4_2, match ? 0xFF2ECC71 : 0xFFE74C3C, bg_color);

        char s_p4_3[128] = "RAW_DMA[0..7]: ";
        for (int i = 0; i < 8; i++) {
            char b_h[8]; u64_to_str(g_usb_phase3.dma_dump[i], b_h);
            str_cat(s_p4_3, b_h); str_cat(s_p4_3, " ");
        }
        abde_render_string(panel_x + 30, panel_y + 590, s_p4_3, 0xFFF1C40F, bg_color);

        char s_p4_4[128] = "CFG_HDR[0..3]: ";
        for (int i = 0; i < 4; i++) {
            char b_h2[8]; u64_to_str(g_usb_phase3.cfg_buf_dump[i], b_h2);
            str_cat(s_p4_4, b_h2); str_cat(s_p4_4, " ");
        }
        str_cat(s_p4_4, " | REJECT: "); str_cat(s_p4_4, g_usb_phase3.reject_reason ? (char*)g_usb_phase3.reject_reason : "NONE");
        abde_render_string(panel_x + 30, panel_y + 610, s_p4_4, 0xFF00FFFF, bg_color);

        char s_p4_5[128] = "FORENSIC_RESULT: "; str_cat(s_p4_5, g_usb_phase3.dma_forensic_result ? (char*)g_usb_phase3.dma_forensic_result : "UNKNOWN");
        abde_render_string(panel_x + 30, panel_y + 630, s_p4_5, 0xFFE74C3C, bg_color);

        // =====================================================================
        // ATOMS OS USB FORENSIC COMMAND CENTER V1.0 — 10-PANEL VISUAL DEEP DEBUG
        // =====================================================================
        #include "kernel/drivers/usb/forensics/usb_forensic_center.h"

        abde_fill_rect(panel_x + 20, panel_y + 650, panel_w - 40, 1, 0xFFBDC3C7);
        abde_render_string(panel_x + 30, panel_y + 655, "--- ATOMS OS USB FORENSIC COMMAND CENTER V1.0 (10-PANEL FEAST DEEP DEBUG) ---", 0xFFE74C3C, bg_color);

        char s_f1[128] = "CTRL: "; str_cat(s_f1, g_forensic_center.dashboard.controller_type);
        str_cat(s_f1, " | STATE: "); str_cat(s_f1, g_forensic_center.dashboard.state);
        str_cat(s_f1, " | USBCMD: 0x"); char b_cmd[16]; u64_to_hex_str(g_forensic_center.dashboard.usbcmd, b_cmd); str_cat(s_f1, b_cmd);
        str_cat(s_f1, " | USBSTS: 0x"); char b_sts[16]; u64_to_hex_str(g_forensic_center.dashboard.usbsts, b_sts); str_cat(s_f1, b_sts);
        abde_render_string(panel_x + 30, panel_y + 675, s_f1, 0xFF2ECC71, bg_color);

        char s_f2[128] = "COMPARE USED: "; str_cat(s_f2, g_forensic_center.comparison.controller_used);
        str_cat(s_f2, " | FALLBACK: "); str_cat(s_f2, g_forensic_center.comparison.fallback_mode);
        str_cat(s_f2, " | MOTHERBOARD: "); str_cat(s_f2, g_forensic_center.hardware.motherboard);
        abde_render_string(panel_x + 30, panel_y + 670, s_f2, 0xFF00FFFF, bg_color);

        char s_f3[128] = "EVT RING TOTAL: "; char b_tot[16]; u64_to_str(g_forensic_center.event_ring.total_events, b_tot); str_cat(s_f3, b_tot);
        str_cat(s_f3, " | LAST IRQ TYPE: "); str_cat(s_f3, g_forensic_center.interrupts.last_event_type[0] ? g_forensic_center.interrupts.last_event_type : "NONE");
        str_cat(s_f3, " | CPU: "); str_cat(s_f3, g_forensic_center.hardware.cpu);
        abde_render_string(panel_x + 30, panel_y + 690, s_f3, 0xFFF1C40F, bg_color);

        // Software Cursor Overlay
        if (cx >= 0 && cx < (int32_t)g_abde.width && cy >= 0 && cy < (int32_t)g_abde.height) {
            abde_fill_rect(cx - 8, cy, 17, 2, 0xFF00FF00);
            abde_fill_rect(cx, cy - 8, 2, 17, 0xFF00FF00);
        }

        scheduler_sleep(50);
    }
}
