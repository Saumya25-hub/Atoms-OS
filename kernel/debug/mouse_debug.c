/*
 * ATOMS OS — Physical Hardware USB HID Mouse Forensic Inspector
 * Real-time on-screen telemetry renderer using ABDE Framebuffer engine.
 */

#include "kernel/debug/mouse_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/usb/core/usb_core.h"

volatile MouseDiagTelemetry g_mouse_diag_telemetry = {0};

/* Internal formatting helpers for freestanding kernel */
static void m_fmt_u32(char *buf, uint32_t val) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char tmp[16];
    int pos = 14;
    tmp[15] = '\0';
    uint32_t v = val;
    while (v > 0) {
        tmp[pos--] = '0' + (v % 10);
        v /= 10;
    }
    strcpy(buf, &tmp[pos + 1]);
}

static void m_fmt_i32(char *buf, int32_t val) {
    if (val < 0) {
        buf[0] = '-';
        m_fmt_u32(buf + 1, (uint32_t)(-val));
    } else {
        m_fmt_u32(buf, (uint32_t)val);
    }
}

static void m_fmt_hex8(char *buf, uint8_t byte) {
    static const char hex[] = "0123456789ABCDEF";
    buf[0] = hex[(byte >> 4) & 0xF];
    buf[1] = hex[byte & 0xF];
    buf[2] = '\0';
}

void mouse_debug_record_packet(uint8_t slot, uint8_t port, uint8_t iface, uint8_t protocol,
                               uint8_t dev_protocol, uint32_t length, const uint8_t* report,
                               bool is_keyboard) {
    if (!report || length == 0) return;

    g_mouse_diag_telemetry.total_packets++;
    if (is_keyboard) {
        g_mouse_diag_telemetry.keyboard_packets++;
    } else {
        g_mouse_diag_telemetry.mouse_packets++;
    }

    uint32_t idx = g_mouse_diag_telemetry.head % MOUSE_DIAG_RING_SIZE;
    g_mouse_diag_telemetry.head++;

    MouseDiagRecord* r = (MouseDiagRecord*)&g_mouse_diag_telemetry.records[idx];
    extern uint64_t timer_get_ticks(void);
    r->timestamp = timer_get_ticks();
    r->slot = slot;
    r->port = port;
    r->iface = iface;
    r->protocol = protocol;
    r->dev_protocol = dev_protocol;
    r->length = length;
    r->is_keyboard = is_keyboard;

    for (uint32_t i = 0; i < 8; i++) {
        r->data[i] = (i < length) ? report[i] : 0;
    }

    if (length >= 3) {
        r->buttons = report[0];
        r->dx = (int8_t)report[1];
        r->dy = (int8_t)report[2];
        r->wheel = (length >= 4) ? (int8_t)report[3] : 0;

        if (!is_keyboard) {
            g_mouse_diag_telemetry.accum_x += r->dx;
            g_mouse_diag_telemetry.accum_y += r->dy;
            g_mouse_diag_telemetry.cur_buttons = r->buttons;
        }
    } else {
        r->buttons = 0;
        r->dx = 0;
        r->dy = 0;
        r->wheel = 0;
    }
}

static bool s_static_rendered = false;

void mouse_debug_reset_layout(void) {
    s_static_rendered = false;
}

void mouse_debug_render_frame(void) {
    /* Colors (ARGB) */
    const uint32_t bg_color     = 0xFF0A0F1D; // Deep Blue/Black
    const uint32_t panel_bg     = 0xFF0F172A; // Slate-900 Panel
    const uint32_t header_bg    = 0xFF1E293B; // Slate-800 Header
    const uint32_t text_title   = 0xFF38BDF8; // Cyan-400 Title
    const uint32_t text_label   = 0xFF94A3B8; // Slate-400 Label
    const uint32_t text_val     = 0xFFF8FAFC; // White Text
    const uint32_t color_green  = 0xFF4ADE80; // Green Pass
    const uint32_t color_red    = 0xFFEF4444; // Red Alert
    const uint32_t color_yellow = 0xFFFBBF24; // Amber Warn

    uint32_t lx = 20;
    uint32_t ly = 80;
    uint32_t lw = 360;
    uint32_t lh = 530;

    uint32_t rx = 395;
    uint32_t ry = 80;
    uint32_t rw = 865;
    uint32_t rh = 530;
    uint32_t tbl_y = ry + 45;

    /* Render static layout only once (or after page switch) to eliminate VRAM bus stall */
    if (!s_static_rendered) {
        /* 1. Clear Framebuffer background */
        abde_fill_rect(0, 0, 1280, 800, bg_color);

        /* 2. Top Title Bar */
        abde_fill_rect(20, 20, 1240, 50, header_bg);
        abde_render_string(35, 28, "ATOMS PHYSICAL HARDWARE FORENSICS -- LIVE USB HID MOUSE INSPECTOR", text_title, header_bg);
        abde_render_string(35, 48, "ASUS PRIME B760M-K (xHCI Host Controller) | Target: Haswell/Raptor Lake Silicon", text_label, header_bg);

        /* 3. Left Panel Static Shell */
        abde_fill_rect(lx, ly, lw, lh, panel_bg);
        abde_render_string(lx + 15, ly + 15, "[DEVICE & PACKET TELEMETRY]", text_title, panel_bg);
        abde_render_string(lx + 15, ly + 45, "Total HID Packets  :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 67, "Mouse Routed (OK)  :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 89, "Keyboard Routed    :", text_label, panel_bg);

        abde_render_string(lx + 15, ly + 125, "[BOUND DRIVER INSTANCES]", text_title, panel_bg);
        abde_render_string(lx + 15, ly + 150, "Mouse Slot / Port  :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 172, "Keyboard Slot/Port :", text_label, panel_bg);

        abde_render_string(lx + 15, ly + 215, "[LIVE CURSOR ACCUMULATOR]", text_title, panel_bg);
        abde_render_string(lx + 15, ly + 240, "Accumulated X      :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 262, "Accumulated Y      :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 284, "Left Button        :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 306, "Right Button       :", text_label, panel_bg);
        abde_render_string(lx + 15, ly + 328, "Middle Button      :", text_label, panel_bg);

        /* 4. Right Panel Static Shell */
        abde_fill_rect(rx, ry, rw, rh, panel_bg);
        abde_render_string(rx + 15, ry + 15, "[LIVE INCOMING USB HID PACKET STREAM -- LAST 12 PACKETS]", text_title, panel_bg);
        abde_fill_rect(rx + 10, tbl_y - 4, rw - 20, 22, header_bg);
        abde_render_string(rx + 15,  tbl_y, "#", text_title, header_bg);
        abde_render_string(rx + 45,  tbl_y, "TIME(ms)", text_title, header_bg);
        abde_render_string(rx + 130, tbl_y, "SLOT:IF", text_title, header_bg);
        abde_render_string(rx + 200, tbl_y, "LEN", text_title, header_bg);
        abde_render_string(rx + 245, tbl_y, "RAW DATA [B0..B7]", text_title, header_bg);
        abde_render_string(rx + 500, tbl_y, "DECODED (dX, dY, BTN)", text_title, header_bg);
        abde_render_string(rx + 710, tbl_y, "ROUTED AS", text_title, header_bg);

        s_static_rendered = true;
    }

    char num_buf[32];
    uint32_t row_y = ly + 45;

    /* Total packets */
    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    m_fmt_u32(num_buf, (uint32_t)g_mouse_diag_telemetry.total_packets);
    abde_render_string(lx + 200, row_y, num_buf, text_val, panel_bg);
    row_y += 22;

    /* Mouse routed */
    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    m_fmt_u32(num_buf, (uint32_t)g_mouse_diag_telemetry.mouse_packets);
    abde_render_string(lx + 200, row_y, num_buf, color_green, panel_bg);
    row_y += 22;

    /* Keyboard routed */
    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    m_fmt_u32(num_buf, (uint32_t)g_mouse_diag_telemetry.keyboard_packets);
    abde_render_string(lx + 200, row_y, num_buf, color_yellow, panel_bg);
    row_y += 36;

    /* Bound devices */
    extern USBDevice* usb_hid_get_mouse_dev(void);
    extern USBDevice* usb_hid_get_keyboard_dev(void);
    USBDevice* m_dev = usb_hid_get_mouse_dev();
    USBDevice* k_dev = usb_hid_get_keyboard_dev();

    row_y += 25;
    abde_fill_rect(lx + 175, row_y, 180, 20, panel_bg);
    if (m_dev) {
        char dev_info[32];
        char slot_s[8], port_s[8], proto_s[8];
        m_fmt_u32(slot_s, m_dev->slot_id);
        m_fmt_u32(port_s, m_dev->port);
        m_fmt_u32(proto_s, m_dev->protocol);
        strcpy(dev_info, "Slot="); strcat(dev_info, slot_s);
        strcat(dev_info, " Port="); strcat(dev_info, port_s);
        strcat(dev_info, " P="); strcat(dev_info, proto_s);
        abde_render_string(lx + 180, row_y, dev_info, color_green, panel_bg);
    } else {
        abde_render_string(lx + 180, row_y, "NOT BOUND (NULL)", color_red, panel_bg);
    }
    row_y += 22;

    abde_fill_rect(lx + 175, row_y, 180, 20, panel_bg);
    if (k_dev) {
        char dev_info[32];
        char slot_s[8], port_s[8], proto_s[8];
        m_fmt_u32(slot_s, k_dev->slot_id);
        m_fmt_u32(port_s, k_dev->port);
        m_fmt_u32(proto_s, k_dev->protocol);
        strcpy(dev_info, "Slot="); strcat(dev_info, slot_s);
        strcat(dev_info, " Port="); strcat(dev_info, port_s);
        strcat(dev_info, " P="); strcat(dev_info, proto_s);
        abde_render_string(lx + 180, row_y, dev_info, color_yellow, panel_bg);
    } else {
        abde_render_string(lx + 180, row_y, "NOT BOUND (NULL)", text_label, panel_bg);
    }
    row_y += 43;

    /* Live Cursor & Button State */
    row_y += 25;
    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    m_fmt_i32(num_buf, g_mouse_diag_telemetry.accum_x);
    abde_render_string(lx + 200, row_y, num_buf, text_val, panel_bg);
    row_y += 22;

    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    m_fmt_i32(num_buf, g_mouse_diag_telemetry.accum_y);
    abde_render_string(lx + 200, row_y, num_buf, text_val, panel_bg);
    row_y += 22;

    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    abde_render_string(lx + 200, row_y, (g_mouse_diag_telemetry.cur_buttons & 0x01) ? "PRESSED" : "UP",
                       (g_mouse_diag_telemetry.cur_buttons & 0x01) ? color_green : text_label, panel_bg);
    row_y += 22;

    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    abde_render_string(lx + 200, row_y, (g_mouse_diag_telemetry.cur_buttons & 0x02) ? "PRESSED" : "UP",
                       (g_mouse_diag_telemetry.cur_buttons & 0x02) ? color_green : text_label, panel_bg);
    row_y += 22;

    abde_fill_rect(lx + 180, row_y, 170, 20, panel_bg);
    abde_render_string(lx + 200, row_y, (g_mouse_diag_telemetry.cur_buttons & 0x04) ? "PRESSED" : "UP",
                       (g_mouse_diag_telemetry.cur_buttons & 0x04) ? color_green : text_label, panel_bg);

    tbl_y += 26;

    uint32_t count = g_mouse_diag_telemetry.head;
    uint32_t displayed = 0;

    for (uint32_t i = 0; i < MOUSE_DIAG_RING_SIZE && i < count; i++) {
        uint32_t idx = (count - 1 - i) % MOUSE_DIAG_RING_SIZE;
        MouseDiagRecord* rec = (MouseDiagRecord*)&g_mouse_diag_telemetry.records[idx];

        uint32_t row_bg = (i % 2 == 0) ? panel_bg : (panel_bg + 0x00040810);
        abde_fill_rect(rx + 10, tbl_y - 3, rw - 20, 22, row_bg);

        /* # index */
        char idx_str[8];
        m_fmt_u32(idx_str, count - i);
        abde_render_string(rx + 15, tbl_y, idx_str, text_label, row_bg);

        /* Timestamp */
        char time_str[16];
        m_fmt_u32(time_str, (uint32_t)(rec->timestamp % 1000000));
        abde_render_string(rx + 45, tbl_y, time_str, text_val, row_bg);

        /* Slot:IF */
        char slot_if[16];
        char s_s[8], if_s[8];
        m_fmt_u32(s_s, rec->slot);
        m_fmt_u32(if_s, rec->iface);
        strcpy(slot_if, s_s); strcat(slot_if, ":"); strcat(slot_if, if_s);
        abde_render_string(rx + 130, tbl_y, slot_if, text_val, row_bg);

        /* Length */
        char len_str[8];
        m_fmt_u32(len_str, rec->length);
        abde_render_string(rx + 200, tbl_y, len_str, text_val, row_bg);

        /* Raw Hex Bytes */
        char hex_dump[32];
        hex_dump[0] = '\0';
        for (uint32_t b = 0; b < rec->length && b < 8; b++) {
            char hb[4];
            m_fmt_hex8(hb, rec->data[b]);
            strcat(hex_dump, hb);
            if (b < 7 && b < rec->length - 1) strcat(hex_dump, " ");
        }
        abde_render_string(rx + 245, tbl_y, hex_dump, color_green, row_bg);

        /* Decoded dX, dY, Btn */
        char dec_str[32];
        char dx_s[8], dy_s[8], btn_s[8];
        m_fmt_i32(dx_s, rec->dx);
        m_fmt_i32(dy_s, rec->dy);
        m_fmt_u32(btn_s, rec->buttons);
        strcpy(dec_str, "X="); strcat(dec_str, dx_s);
        strcat(dec_str, " Y="); strcat(dec_str, dy_s);
        strcat(dec_str, " B="); strcat(dec_str, btn_s);
        abde_render_string(rx + 500, tbl_y, dec_str, text_val, row_bg);

        /* Routing status */
        if (rec->is_keyboard) {
            /* Check if packet actually carried mouse delta */
            if (rec->length <= 5 && (rec->dx != 0 || rec->dy != 0 || rec->buttons != 0)) {
                abde_render_string(rx + 710, tbl_y, "MISROUTED->KBD!", color_red, row_bg);
            } else {
                abde_render_string(rx + 710, tbl_y, "KEYBOARD", color_yellow, row_bg);
            }
        } else {
            abde_render_string(rx + 710, tbl_y, "MOUSE (OK)", color_green, row_bg);
        }

        tbl_y += 24;
        displayed++;
    }

    if (displayed == 0) {
        abde_render_string(rx + 250, tbl_y + 30, "NO USB HID PACKETS RECEIVED YET", color_yellow, panel_bg);
        abde_render_string(rx + 220, tbl_y + 55, "MOVE OR CLICK PHYSICAL USB MOUSE TO TRIGGER PACKETS", text_label, panel_bg);
    }

    /* 5. Bottom Navigation Footer */
    abde_fill_rect(20, 625, 1240, 45, header_bg);
    abde_render_string(35, 638, "[F1 / ESC] VMX Hypervisor Dashboard", text_title, header_bg);
    abde_render_string(380, 638, "[F8] Live Mouse Inspector (Current)", color_green, header_bg);
    abde_render_string(720, 638, "[F9] Keyboard Inspector", text_val, header_bg);
}

