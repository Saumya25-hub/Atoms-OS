#include "usb_hid_led_debug.h"
#include "kernel/debug/aipdebug/aipdebug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "kernel/drivers/usb/core/usb_core.h"
#include "arch/x86_64/io/port_io.h"

extern USBDevice* usb_hid_get_keyboard_device(void);
extern void usb_hid_set_leds_ex(USBDevice* dev, uint8_t leds, const char* reason);
extern void usb_hid_set_leds(USBDevice* dev, uint8_t leds);
extern void usb_hid_sync_leds(void);

typedef struct {
    bool has_report_id;
    uint8_t report_id;
    uint8_t num_lock_bit;
    uint8_t caps_lock_bit;
    uint8_t scroll_lock_bit;
    bool has_num_lock;
    bool has_caps_lock;
    bool has_scroll_lock;
    bool parsed;
} HIDLedLayout;

extern const HIDLedLayout* usb_hid_get_led_layout(void);
extern uint8_t g_raw_report_desc[512];
extern uint16_t g_raw_report_desc_len;
extern bool g_kbd_has_interrupt_out;
extern uint8_t g_kbd_out_ep;
extern uint16_t g_kbd_out_max_pkt;
extern volatile uint8_t g_kbd_interface_num;
extern volatile uint8_t g_kbd_ep_addr;
extern volatile bool g_caps_lock_state;
extern volatile bool g_num_lock_state;
extern volatile bool g_scroll_lock_state;
extern volatile uint8_t g_last_key_usage;

typedef struct {
    uint64_t timestamp;
    const char* reason;
    bool num_state;
    bool caps_state;
    bool scroll_state;
    uint8_t report_byte;
    uint8_t iface;
    uint16_t wValue;
    bool success;
    uint32_t completion_code;
} LedAuditLogEntry;

#define LED_LOG_MAX 20
extern LedAuditLogEntry g_led_audit_log[LED_LOG_MAX];
extern uint32_t g_led_audit_log_count;

UsbHidLedAudit g_usb_hid_led_audit;

static inline void com1_putc(char c) {
    while ((io_in8(0x3F8 + 5) & 0x20) == 0);
    io_out8(0x3F8, c);
}

static void com1_puts(const char *s) {
    while (*s) {
        if (*s == '\n') com1_putc('\r');
        com1_putc(*s++);
    }
}

static void com1_put_hex_byte(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    com1_putc(hex[(val >> 4) & 0xF]);
    com1_putc(hex[val & 0xF]);
}

static void com1_put_hex_word(uint16_t val) {
    com1_put_hex_byte(val >> 8);
    com1_put_hex_byte(val & 0xFF);
}

static void com1_put_dec(uint32_t val) {
    if (val == 0) { com1_putc('0'); return; }
    char buf[12]; int pos = 10; buf[11] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

// =====================================================================
// STEP 1: USB KEYBOARD DISCOVERY & DESCRIPTOR AUDIT
// =====================================================================
static USBDevice* audit_locate_usb_keyboard(void) {
    USBDevice* kbd = usb_hid_get_keyboard_device();

    if (!kbd) {
        com1_puts("[USB-LED-AUDIT] [FAIL] No USB HID Keyboard found in device registry!\r\n");
        g_usb_hid_led_audit.keyboard_found = false;
        return NULL;
    }

    g_usb_hid_led_audit.keyboard_found = true;
    g_usb_hid_led_audit.address = kbd->address;
    g_usb_hid_led_audit.slot_id = kbd->slot_id;
    g_usb_hid_led_audit.vid = kbd->vid;
    g_usb_hid_led_audit.pid = kbd->pid;
    g_usb_hid_led_audit.interface_num = g_kbd_interface_num;
    g_usb_hid_led_audit.protocol = kbd->protocol;
    g_usb_hid_led_audit.subclass = 1;

    g_usb_hid_led_audit.interrupt_in_ep = g_kbd_ep_addr ? g_kbd_ep_addr : 1;
    g_usb_hid_led_audit.interrupt_in_max_pkt = 8;

    g_usb_hid_led_audit.has_interrupt_out = g_kbd_has_interrupt_out;
    g_usb_hid_led_audit.interrupt_out_ep = g_kbd_out_ep;
    g_usb_hid_led_audit.interrupt_out_max_pkt = g_kbd_out_max_pkt;

    if (g_kbd_has_interrupt_out) {
        g_usb_hid_led_audit.out_transport_type = "INTERRUPT OUT (EP Defined)";
    } else {
        g_usb_hid_led_audit.out_transport_type = "CONTROL SET_REPORT (EP0: bRequest 0x09)";
    }

    com1_puts("[USB-LED-AUDIT] Keyboard Identified: VID=0x");
    com1_put_hex_word(kbd->vid);
    com1_puts(" PID=0x");
    com1_put_hex_word(kbd->pid);
    com1_puts(" Addr=");
    com1_put_dec(kbd->address);
    com1_puts(" Slot=");
    com1_put_dec(kbd->slot_id);
    com1_puts(" Iface=");
    com1_put_dec(g_kbd_interface_num);
    com1_puts(" IN_EP=");
    com1_put_dec(g_usb_hid_led_audit.interrupt_in_ep);
    com1_puts(" OUT_EP=");
    com1_put_dec(g_usb_hid_led_audit.interrupt_out_ep);
    com1_puts("\r\n");

    return kbd;
}

// Helper to format integer to string
static void u32_to_str(uint32_t val, char* out) {
    if (val == 0) { out[0] = '0'; out[1] = '\0'; return; }
    char buf[12]; int pos = 10; buf[11] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    int i = 0;
    for (int p = pos + 1; p <= 11; p++) {
        out[i++] = buf[p];
    }
}

// =====================================================================
// STEP 2: LIVE FORENSIC DASHBOARD RENDERING
// =====================================================================
// Colors
static const uint32_t s_panel_bg    = 0x00080E1A; // Dark Navy Slate
static const uint32_t s_header_bg   = 0x00101C30;
static const uint32_t s_text_color  = 0x00E2E8F0;
static const uint32_t s_label_color = 0x0094A3B8;
static const uint32_t s_pass_color  = 0x0022C55E; // Emerald
static const uint32_t s_fail_color  = 0x00EF4444; // Ruby
static const uint32_t s_warn_color  = 0x00F59E0B; // Amber
static const uint32_t s_cyan_color  = 0x0006B6D4;
static const uint32_t s_white_color = 0x00FFFFFF;

static void audit_render_heartbeat(char spinner_char, uint32_t uptime_sec) {
    abde_fill_rect(1340, 24, 540, 40, s_header_bg);
    char hb_str[64];
    hb_str[0] = 'A'; hb_str[1] = 'T'; hb_str[2] = 'O'; hb_str[3] = 'M'; hb_str[4] = 'S'; hb_str[5] = ' ';
    hb_str[6] = 'L'; hb_str[7] = 'I'; hb_str[8] = 'V'; hb_str[9] = 'E'; hb_str[10] = ' ';
    hb_str[11] = '['; hb_str[12] = spinner_char; hb_str[13] = ']'; hb_str[14] = ' ';
    hb_str[15] = '|'; hb_str[16] = ' ';
    hb_str[17] = 'U'; hb_str[18] = 'p'; hb_str[19] = ':'; hb_str[20] = ' ';
    char up_buf[16];
    u32_to_str(uptime_sec, up_buf);
    int pos = 21;
    for (int i = 0; up_buf[i]; i++) hb_str[pos++] = up_buf[i];
    hb_str[pos++] = 's'; hb_str[pos] = '\0';
    abde_render_string(1360, 36, hb_str, s_pass_color, s_header_bg);
}

static void audit_render_static_layout(void) {
    abde_fill_rect(0, 0, 1920, 1080, 0x00020617);
    abde_fill_rect(20, 20, 1880, 1040, s_panel_bg);

    // Title Bar
    abde_fill_rect(20, 20, 1880, 48, s_header_bg);
    abde_render_string(36, 36, "ATOMS OS — PHYSICAL USB HID KEYBOARD LED FORENSIC CONSOLE", s_cyan_color, s_header_bg);

    // Left Column: Physical Keyboard Identity & Endpoint Configuration
    int lx = 40;
    int ly = 80;

    abde_render_string(lx, ly, "1. PHYSICAL USB KEYBOARD HARDWARE IDENTIFICATION", s_cyan_color, s_panel_bg);
    ly += 22;

    abde_render_string(lx + 10, ly, "USB Keyboard Status     :", s_label_color, s_panel_bg);
    abde_render_string(lx + 230, ly, g_usb_hid_led_audit.keyboard_found ? "DETECTED & ENUMERATED" : "NOT DETECTED", g_usb_hid_led_audit.keyboard_found ? s_pass_color : s_fail_color, s_panel_bg);
    ly += 18;

    char vidpid[32];
    const char hex[] = "0123456789ABCDEF";
    vidpid[0] = '0'; vidpid[1] = 'x';
    vidpid[2] = hex[(g_usb_hid_led_audit.vid >> 12) & 0xF];
    vidpid[3] = hex[(g_usb_hid_led_audit.vid >> 8) & 0xF];
    vidpid[4] = hex[(g_usb_hid_led_audit.vid >> 4) & 0xF];
    vidpid[5] = hex[g_usb_hid_led_audit.vid & 0xF];
    vidpid[6] = ' '; vidpid[7] = ':'; vidpid[8] = ' ';
    vidpid[9] = '0'; vidpid[10] = 'x';
    vidpid[11] = hex[(g_usb_hid_led_audit.pid >> 12) & 0xF];
    vidpid[12] = hex[(g_usb_hid_led_audit.pid >> 8) & 0xF];
    vidpid[13] = hex[(g_usb_hid_led_audit.pid >> 4) & 0xF];
    vidpid[14] = hex[g_usb_hid_led_audit.pid & 0xF];
    vidpid[15] = '\0';
    abde_render_string(lx + 10, ly, "Vendor ID / Product ID  :", s_label_color, s_panel_bg);
    abde_render_string(lx + 230, ly, vidpid, s_pass_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "USB Address / Slot ID   :", s_label_color, s_panel_bg);
    char addrslot[32];
    addrslot[0] = 'A'; addrslot[1] = 'd'; addrslot[2] = 'd'; addrslot[3] = 'r'; addrslot[4] = '=';
    addrslot[5] = '0' + g_usb_hid_led_audit.address; addrslot[6] = ' ';
    addrslot[7] = 'S'; addrslot[8] = 'l'; addrslot[9] = 'o'; addrslot[10] = 't'; addrslot[11] = '=';
    addrslot[12] = '0' + g_usb_hid_led_audit.slot_id; addrslot[13] = '\0';
    abde_render_string(lx + 230, ly, addrslot, s_text_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Active Keyboard Iface   :", s_label_color, s_panel_bg);
    char iface_str[32];
    iface_str[0] = 'I'; iface_str[1] = 'F'; iface_str[2] = ' ';
    iface_str[3] = '0' + g_kbd_interface_num;
    iface_str[4] = ' '; iface_str[5] = '('; iface_str[6] = 'P'; iface_str[7] = 'r'; iface_str[8] = 'o';
    iface_str[9] = 't'; iface_str[10] = 'o'; iface_str[11] = 'c'; iface_str[12] = 'o'; iface_str[13] = 'l';
    iface_str[14] = ' '; iface_str[15] = '1'; iface_str[16] = ' '; iface_str[17] = 'K'; iface_str[18] = 'b';
    iface_str[19] = 'd'; iface_str[20] = ')'; iface_str[21] = '\0';
    abde_render_string(lx + 230, ly, iface_str, s_pass_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Interrupt IN Endpoint   :", s_label_color, s_panel_bg);
    char in_ep_str[32];
    in_ep_str[0] = 'E'; in_ep_str[1] = 'P'; in_ep_str[2] = ' ';
    in_ep_str[3] = '0' + g_usb_hid_led_audit.interrupt_in_ep;
    in_ep_str[4] = ' '; in_ep_str[5] = '('; in_ep_str[6] = '8'; in_ep_str[7] = 'B'; in_ep_str[8] = ' ';
    in_ep_str[9] = 'P'; in_ep_str[10] = 'a'; in_ep_str[11] = 'c'; in_ep_str[12] = 'k'; in_ep_str[13] = 'e';
    in_ep_str[14] = 't'; in_ep_str[15] = ')'; in_ep_str[16] = '\0';
    abde_render_string(lx + 230, ly, in_ep_str, s_text_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Interrupt OUT Endpoint  :", s_label_color, s_panel_bg);
    if (g_usb_hid_led_audit.has_interrupt_out) {
        char out_ep_str[32];
        out_ep_str[0] = 'E'; out_ep_str[1] = 'P'; out_ep_str[2] = ' ';
        out_ep_str[3] = '0' + g_usb_hid_led_audit.interrupt_out_ep;
        out_ep_str[4] = '\0';
        abde_render_string(lx + 230, ly, out_ep_str, s_pass_color, s_panel_bg);
    } else {
        abde_render_string(lx + 230, ly, "NONE (EP0 Control SET_REPORT Used)", s_warn_color, s_panel_bg);
    }
    ly += 18;

    abde_render_string(lx + 10, ly, "LED OUT Transport Model :", s_label_color, s_panel_bg);
    abde_render_string(lx + 230, ly, g_usb_hid_led_audit.has_interrupt_out ? "INTERRUPT OUT (EPx)" : "CONTROL SET_REPORT (EP0: bRequest 0x09)", s_cyan_color, s_panel_bg);
    ly += 30;

    // Section 2: HID Report Descriptor
    abde_render_string(lx, ly, "2. AUTHORITATIVE HID REPORT DESCRIPTOR (PARSED & DUMPED)", s_cyan_color, s_panel_bg);
    ly += 22;

    const HIDLedLayout* layout = usb_hid_get_led_layout();

    abde_render_string(lx + 10, ly, "Report Descriptor Length:", s_label_color, s_panel_bg);
    char len_buf[32];
    u32_to_str(g_raw_report_desc_len, len_buf);
    int lp = 0; while (len_buf[lp]) lp++;
    len_buf[lp++] = ' '; len_buf[lp++] = 'B'; len_buf[lp++] = 'y'; len_buf[lp++] = 't'; len_buf[lp++] = 'e'; len_buf[lp++] = 's'; len_buf[lp] = '\0';
    abde_render_string(lx + 230, ly, len_buf, s_text_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Report ID / Length      :", s_label_color, s_panel_bg);
    if (layout->has_report_id) {
        char rpt_id_str[32];
        rpt_id_str[0] = 'I'; rpt_id_str[1] = 'D'; rpt_id_str[2] = ':'; rpt_id_str[3] = ' ';
        rpt_id_str[4] = '0' + layout->report_id; rpt_id_str[5] = '\0';
        abde_render_string(lx + 230, ly, rpt_id_str, s_warn_color, s_panel_bg);
    } else {
        abde_render_string(lx + 230, ly, "Report ID: None (0) | Length: 1 Byte", s_text_color, s_panel_bg);
    }
    ly += 18;

    abde_render_string(lx + 10, ly, "Mapped LED Usages       :", s_label_color, s_panel_bg);
    char bits_str[64];
    bits_str[0] = 'N'; bits_str[1] = 'u'; bits_str[2] = 'm'; bits_str[3] = ':'; bits_str[4] = 'B';
    bits_str[5] = '0' + layout->num_lock_bit; bits_str[6] = ' '; bits_str[7] = '|'; bits_str[8] = ' ';
    bits_str[9] = 'C'; bits_str[10] = 'a'; bits_str[11] = 'p'; bits_str[12] = 's'; bits_str[13] = ':'; bits_str[14] = 'B';
    bits_str[15] = '0' + layout->caps_lock_bit; bits_str[16] = ' '; bits_str[17] = '|'; bits_str[18] = ' ';
    bits_str[19] = 'S'; bits_str[20] = 'c'; bits_str[21] = 'r'; bits_str[22] = ':'; bits_str[23] = 'B';
    bits_str[24] = '0' + layout->scroll_lock_bit; bits_str[25] = '\0';
    abde_render_string(lx + 230, ly, bits_str, s_pass_color, s_panel_bg);
    ly += 22;

    abde_render_string(lx + 10, ly, "Raw Report Desc (Hex Dump 0..31):", s_label_color, s_panel_bg);
    ly += 18;
    char hex_dump1[80];
    int hp = 0;
    for (int i = 0; i < 16 && i < g_raw_report_desc_len; i++) {
        hex_dump1[hp++] = hex[(g_raw_report_desc[i] >> 4) & 0xF];
        hex_dump1[hp++] = hex[g_raw_report_desc[i] & 0xF];
        hex_dump1[hp++] = ' ';
    }
    hex_dump1[hp] = '\0';
    abde_render_string(lx + 10, ly, hex_dump1, s_cyan_color, s_panel_bg);
    ly += 18;

    char hex_dump2[80];
    hp = 0;
    for (int i = 16; i < 32 && i < g_raw_report_desc_len; i++) {
        hex_dump2[hp++] = hex[(g_raw_report_desc[i] >> 4) & 0xF];
        hex_dump2[hp++] = hex[g_raw_report_desc[i] & 0xF];
        hex_dump2[hp++] = ' ';
    }
    hex_dump2[hp] = '\0';
    abde_render_string(lx + 10, ly, hex_dump2, s_cyan_color, s_panel_bg);
    ly += 30;

    // Section 5 Header in Left Column
    abde_render_string(lx, ly, "5. REAL-TIME XHCI HARDWARE PIPELINE TELEMETRY", s_cyan_color, s_panel_bg);

    // Right Column Section Headers
    int rx = 960;
    abde_render_string(rx, 80, "3. LIVE MANUAL SINGLE-TOGGLE FORENSIC STATE", s_cyan_color, s_panel_bg);
    abde_render_string(rx, 340, "4. TRANSACTION AUDIT LOG (LAST SYNCHRONIZATIONS)", s_cyan_color, s_panel_bg);

    abde_fill_rect(rx + 10, 362, 910, 22, s_header_bg);
    abde_render_string(rx + 15, 366, "#", s_cyan_color, s_header_bg);
    abde_render_string(rx + 50, 366, "Reason", s_cyan_color, s_header_bg);
    abde_render_string(rx + 220, 366, "Num", s_cyan_color, s_header_bg);
    abde_render_string(rx + 280, 366, "Caps", s_cyan_color, s_header_bg);
    abde_render_string(rx + 340, 366, "Scr", s_cyan_color, s_header_bg);
    abde_render_string(rx + 400, 366, "Data", s_cyan_color, s_header_bg);
    abde_render_string(rx + 480, 366, "Iface", s_cyan_color, s_header_bg);
    abde_render_string(rx + 560, 366, "Code", s_cyan_color, s_header_bg);
    abde_render_string(rx + 650, 366, "Status", s_cyan_color, s_header_bg);
}

static void audit_render_dynamic_state(void) {
    const char hex[] = "0123456789ABCDEF";
    int rx = 960;
    int ry = 104;

    // Clear Section 3 content area
    abde_fill_rect(rx + 10, ry, 910, 225, s_panel_bg);

    // Software Lock States
    abde_render_string(rx + 10, ry, "Software Lock States    :", s_label_color, s_panel_bg);
    abde_render_string(rx + 230, ry, "Num: ", s_label_color, s_panel_bg);
    abde_render_string(rx + 270, ry, g_num_lock_state ? "[ON]" : "[OFF]", g_num_lock_state ? s_pass_color : s_label_color, s_panel_bg);
    abde_render_string(rx + 330, ry, "Caps: ", s_label_color, s_panel_bg);
    abde_render_string(rx + 380, ry, g_caps_lock_state ? "[ON]" : "[OFF]", g_caps_lock_state ? s_pass_color : s_label_color, s_panel_bg);
    abde_render_string(rx + 440, ry, "Scroll: ", s_label_color, s_panel_bg);
    abde_render_string(rx + 500, ry, g_scroll_lock_state ? "[ON]" : "[OFF]", g_scroll_lock_state ? s_pass_color : s_label_color, s_panel_bg);
    ry += 20;

    // Last Keypress Usage ID
    abde_render_string(rx + 10, ry, "Last Physical Key Usage :", s_label_color, s_panel_bg);
    char key_str[32];
    key_str[0] = '0'; key_str[1] = 'x';
    key_str[2] = hex[(g_last_key_usage >> 4) & 0xF];
    key_str[3] = hex[g_last_key_usage & 0xF];
    key_str[4] = ' ';
    if (g_last_key_usage == 0x39) {
        key_str[5] = '('; key_str[6] = 'C'; key_str[7] = 'a'; key_str[8] = 'p'; key_str[9] = 's'; key_str[10] = 'L'; key_str[11] = 'o'; key_str[12] = 'c'; key_str[13] = 'k'; key_str[14] = ')'; key_str[15] = '\0';
    } else if (g_last_key_usage == 0x53) {
        key_str[5] = '('; key_str[6] = 'N'; key_str[7] = 'u'; key_str[8] = 'm'; key_str[9] = 'L'; key_str[10] = 'o'; key_str[11] = 'c'; key_str[12] = 'k'; key_str[13] = ')'; key_str[14] = '\0';
    } else if (g_last_key_usage == 0x47) {
        key_str[5] = '('; key_str[6] = 'S'; key_str[7] = 'c'; key_str[8] = 'r'; key_str[9] = 'o'; key_str[10] = 'l'; key_str[11] = 'l'; key_str[12] = ')'; key_str[13] = '\0';
    } else {
        key_str[5] = '\0';
    }
    abde_render_string(rx + 230, ry, key_str, (g_last_key_usage == 0x39 || g_last_key_usage == 0x53) ? s_pass_color : s_text_color, s_panel_bg);
    ry += 20;

    LedAuditLogEntry* last_log = NULL;
    if (g_led_audit_log_count > 0) {
        uint32_t last_idx = (g_led_audit_log_count - 1) % LED_LOG_MAX;
        last_log = &g_led_audit_log[last_idx];
    }

    abde_render_string(rx + 10, ry, "Last Sync Reason        :", s_label_color, s_panel_bg);
    abde_render_string(rx + 230, ry, last_log ? last_log->reason : "NONE", s_warn_color, s_panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "USB Setup Request (EP0) :", s_label_color, s_panel_bg);
    char req_fields[64];
    req_fields[0] = 'b'; req_fields[1] = 'm'; req_fields[2] = 'R'; req_fields[3] = 'e'; req_fields[4] = 'q'; req_fields[5] = '=';
    req_fields[6] = '0'; req_fields[7] = 'x'; req_fields[8] = '2'; req_fields[9] = '1'; req_fields[10] = ' ';
    req_fields[11] = 'b'; req_fields[12] = 'R'; req_fields[13] = 'e'; req_fields[14] = 'q'; req_fields[15] = '=';
    req_fields[16] = '0'; req_fields[17] = 'x'; req_fields[18] = '0'; req_fields[19] = '9'; req_fields[20] = ' ';
    req_fields[21] = 'w'; req_fields[22] = 'V'; req_fields[23] = 'a'; req_fields[24] = 'l'; req_fields[25] = '=';
    req_fields[26] = '0'; req_fields[27] = 'x'; req_fields[28] = '0'; req_fields[29] = '2'; req_fields[30] = '0';
    req_fields[31] = '0' + (last_log ? (last_log->wValue & 0xF) : 0); req_fields[32] = ' ';
    req_fields[33] = 'w'; req_fields[34] = 'I'; req_fields[35] = 'd'; req_fields[36] = 'x'; req_fields[37] = '=';
    req_fields[38] = '0' + g_kbd_interface_num; req_fields[39] = '\0';
    abde_render_string(rx + 230, ry, req_fields, s_cyan_color, s_panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "Generated Report Byte   :", s_label_color, s_panel_bg);
    char rep_byte_str[32];
    uint8_t rb = last_log ? last_log->report_byte : 0;
    rep_byte_str[0] = '0'; rep_byte_str[1] = 'x';
    rep_byte_str[2] = hex[(rb >> 4) & 0xF];
    rep_byte_str[3] = hex[rb & 0xF];
    rep_byte_str[4] = ' '; rep_byte_str[5] = '(';
    rep_byte_str[6] = 'N'; rep_byte_str[7] = '='; rep_byte_str[8] = (rb & 1) ? '1' : '0'; rep_byte_str[9] = ',';
    rep_byte_str[10] = 'C'; rep_byte_str[11] = '='; rep_byte_str[12] = (rb & 2) ? '1' : '0'; rep_byte_str[13] = ',';
    rep_byte_str[14] = 'S'; rep_byte_str[15] = '='; rep_byte_str[16] = (rb & 4) ? '1' : '0'; rep_byte_str[17] = ')';
    rep_byte_str[18] = '\0';
    abde_render_string(rx + 230, ry, rep_byte_str, s_pass_color, s_panel_bg);
    ry += 18;

    abde_render_string(rx + 10, ry, "xHCI EP0 Transfer Code  :", s_label_color, s_panel_bg);
    if (last_log) {
        char code_str[32];
        code_str[0] = '0' + (last_log->completion_code / 10);
        code_str[1] = '0' + (last_log->completion_code % 10);
        code_str[2] = ' '; code_str[3] = '(';
        if (last_log->completion_code == 1) {
            code_str[4] = 'T'; code_str[5] = 'R'; code_str[6] = 'B'; code_str[7] = '_';
            code_str[8] = 'S'; code_str[9] = 'U'; code_str[10] = 'C'; code_str[11] = 'C';
            code_str[12] = 'E'; code_str[13] = 'S'; code_str[14] = 'S'; code_str[15] = ')';
            code_str[16] = '\0';
            abde_render_string(rx + 230, ry, code_str, s_pass_color, s_panel_bg);
        } else {
            code_str[4] = 'E'; code_str[5] = 'R'; code_str[6] = 'R'; code_str[7] = 'O';
            code_str[8] = 'R'; code_str[9] = ')'; code_str[10] = '\0';
            abde_render_string(rx + 230, ry, code_str, s_fail_color, s_panel_bg);
        }
    } else {
        abde_render_string(rx + 230, ry, "NO TRANSFERS YET", s_label_color, s_panel_bg);
    }
    ry += 20;

    abde_render_string(rx + 10, ry, "Physical LED Status     :", s_label_color, s_panel_bg);
    abde_render_string(rx + 230, ry, "[WAITING OPERATOR DIRECT PHYSICAL OBSERVATION]", s_warn_color, s_panel_bg);

    // Clear Section 4 table rows area
    abde_fill_rect(rx + 10, 388, 910, 180, s_panel_bg);
    int table_y = 388;
    uint32_t count = g_led_audit_log_count;
    uint32_t start = (count > 8) ? (count - 8) : 0;
    for (uint32_t c = start; c < count; c++) {
        uint32_t idx = c % LED_LOG_MAX;
        LedAuditLogEntry* entry = &g_led_audit_log[idx];

        char num_str[8];
        u32_to_str(c + 1, num_str);
        abde_render_string(rx + 15, table_y, num_str, s_text_color, s_panel_bg);
        abde_render_string(rx + 50, table_y, entry->reason ? entry->reason : "UNKNOWN", s_warn_color, s_panel_bg);
        abde_render_string(rx + 220, table_y, entry->num_state ? "ON" : "OFF", entry->num_state ? s_pass_color : s_label_color, s_panel_bg);
        abde_render_string(rx + 280, table_y, entry->caps_state ? "ON" : "OFF", entry->caps_state ? s_pass_color : s_label_color, s_panel_bg);
        abde_render_string(rx + 340, table_y, entry->scroll_state ? "ON" : "OFF", entry->scroll_state ? s_pass_color : s_label_color, s_panel_bg);

        char d_str[8];
        d_str[0] = '0'; d_str[1] = 'x';
        d_str[2] = hex[(entry->report_byte >> 4) & 0xF];
        d_str[3] = hex[entry->report_byte & 0xF];
        d_str[4] = '\0';
        abde_render_string(rx + 400, table_y, d_str, s_text_color, s_panel_bg);

        char if_str[8];
        if_str[0] = '0' + entry->iface; if_str[1] = '\0';
        abde_render_string(rx + 480, table_y, if_str, s_text_color, s_panel_bg);

        char c_str[8];
        c_str[0] = '0' + (entry->completion_code % 10); c_str[1] = '\0';
        abde_render_string(rx + 560, table_y, c_str, entry->completion_code == 1 ? s_pass_color : s_fail_color, s_panel_bg);

        abde_render_string(rx + 650, table_y, entry->success ? "SUBMITTED_OK" : "ERROR", entry->success ? s_pass_color : s_fail_color, s_panel_bg);
        table_y += 18;
    }

    // Section 5: Real-Time xHCI Hardware Telemetry in Left Column
    int lx = 40;
    int ly = 432;
    abde_fill_rect(lx + 10, ly, 880, 160, s_panel_bg);

    extern volatile uint64_t g_xhci_transfers;
    extern volatile uint64_t g_usb_reports_count;
    extern volatile uint32_t g_cfg_last_completion_code;
    extern volatile uint32_t g_cfg_last_slot_id;
    extern volatile uint32_t g_cfg_last_ep_id;
    extern volatile uint32_t g_xhci_last_cmd_completion_code;

    abde_render_string(lx + 10, ly, "Configure EP Command    :", s_label_color, s_panel_bg);
    char cfg_cmd_buf[32];
    cfg_cmd_buf[0] = 'C'; cfg_cmd_buf[1] = 'o'; cfg_cmd_buf[2] = 'd'; cfg_cmd_buf[3] = 'e'; cfg_cmd_buf[4] = '=';
    u32_to_str(g_xhci_last_cmd_completion_code, cfg_cmd_buf + 5);
    abde_render_string(lx + 230, ly, cfg_cmd_buf, g_xhci_last_cmd_completion_code == 1 ? s_pass_color : s_fail_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "xHCI Transfer Events    :", s_label_color, s_panel_bg);
    char xfer_buf[32];
    u32_to_str((uint32_t)g_xhci_transfers, xfer_buf);
    abde_render_string(lx + 230, ly, xfer_buf, g_xhci_transfers > 0 ? s_pass_color : s_warn_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "USB Reports Received    :", s_label_color, s_panel_bg);
    char rpt_cnt_buf[32];
    u32_to_str((uint32_t)g_usb_reports_count, rpt_cnt_buf);
    abde_render_string(lx + 230, ly, rpt_cnt_buf, g_usb_reports_count > 0 ? s_pass_color : s_warn_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Last Event Slot / DCI   :", s_label_color, s_panel_bg);
    char slot_ep_buf[32];
    slot_ep_buf[0] = 'S'; slot_ep_buf[1] = 'l'; slot_ep_buf[2] = 'o'; slot_ep_buf[3] = 't'; slot_ep_buf[4] = '=';
    u32_to_str(g_cfg_last_slot_id, slot_ep_buf + 5);
    int sep = 5; while (slot_ep_buf[sep]) sep++;
    slot_ep_buf[sep++] = ' '; slot_ep_buf[sep++] = 'D'; slot_ep_buf[sep++] = 'C'; slot_ep_buf[sep++] = 'I'; slot_ep_buf[sep++] = '=';
    u32_to_str(g_cfg_last_ep_id, slot_ep_buf + sep);
    abde_render_string(lx + 230, ly, slot_ep_buf, s_cyan_color, s_panel_bg);
    ly += 18;

    abde_render_string(lx + 10, ly, "Last Event Code         :", s_label_color, s_panel_bg);
    char evt_code_buf[32];
    evt_code_buf[0] = 'C'; evt_code_buf[1] = 'o'; evt_code_buf[2] = 'd'; evt_code_buf[3] = 'e'; evt_code_buf[4] = '=';
    u32_to_str(g_cfg_last_completion_code, evt_code_buf + 5);
    abde_render_string(lx + 230, ly, evt_code_buf, g_cfg_last_completion_code == 1 ? s_pass_color : s_warn_color, s_panel_bg);
}

static inline bool is_running_in_vm(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (ecx & (1U << 31)) != 0;
}

// =====================================================================
// MAIN AUDIT RUNNER (LIVE INTERACTIVE MODE)
// =====================================================================
void usb_hid_led_debug_run(boot_info_t *boot_info) {
    (void)boot_info;
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    com1_puts("\r\n========================================================\r\n");
    com1_puts("[USB-LED-AUDIT] LIVE INTERACTIVE PHYSICAL USB HID LED CONSOLE\r\n");
    com1_puts("========================================================\r\n\r\n");

    // Initialize AI-(P)DEBUG telemetry
    aipd_init(AIPD_PROF_USB_XHCI | AIPD_PROF_PS2_KEYBOARD);

    // 1. Locate enumerated USB keyboard
    USBDevice* kbd = audit_locate_usb_keyboard();
    (void)kbd;

    com1_puts("[USB-LED-AUDIT] Entering live interactive loop...\r\n");

    const char spinner_chars[] = {'|', '/', '-', '\\'};
    uint64_t tick = 0;
    uint32_t spinner_idx = 0;
    uint32_t uptime_sec = 0;
    uint32_t last_log_count = 0;
    uint64_t last_screen_tick = 0;
    uint32_t last_shot_sec = 0;

    // Initial static render (Rendered ONCE at boot, never wiped again)
    audit_render_static_layout();
    audit_render_dynamic_state();
    audit_render_heartbeat(spinner_chars[spinner_idx], uptime_sec);
    aipd_flush();

    // Arm initial non-blocking screenshot request (only on bare-metal physical hardware)
    if (!is_running_in_vm()) {
        extern bool atoms_screenshot_request(uint32_t session_id);
        atoms_screenshot_request(1);
    }

    for (;;) {
        tick++;

        // 1. Poll xHCI transfers (Incoming Interrupt IN packets from Keyboard & Mouse)
        extern void xhci_poll(void);
        xhci_poll();

        // 2. Poll Realtek PCIe NIC (Incoming UDP packets for reboot/shutdown)
        if ((tick % 250) == 0) {
            extern void r8168_poll_receive(void);
            r8168_poll_receive();
        }

        // 3. Step cooperative non-blocking screenshot streamer (sends up to 4 chunks = ~5.6KB, <50us)
        if (!is_running_in_vm()) {
            extern bool atoms_screenshot_is_busy(void);
            extern bool atoms_screenshot_step(void);
            if (atoms_screenshot_is_busy()) {
                atoms_screenshot_step();
            }
        }

        // Advance spinner every 15,000 ticks (Zero flicker: only touches 540x40 title bar region)
        if ((tick % 15000) == 0) {
            spinner_idx = (spinner_idx + 1) % 4;
            audit_render_heartbeat(spinner_chars[spinner_idx], uptime_sec);
        }

        // Advance uptime approximately every 300,000 ticks
        if ((tick % 300000) == 0) {
            uptime_sec++;
            audit_render_heartbeat(spinner_chars[spinner_idx], uptime_sec);
        }

        // Check if a new LED event was triggered by keypress
        bool new_event = (g_led_audit_log_count != last_log_count);
        if (new_event) {
            last_log_count = g_led_audit_log_count;
            com1_puts("[USB-LED-AUDIT] New LED Event Registered: Count=");
            com1_put_dec(last_log_count);
            com1_puts(" Reason=");
            uint32_t idx = (last_log_count - 1) % LED_LOG_MAX;
            com1_puts(g_led_audit_log[idx].reason ? g_led_audit_log[idx].reason : "NONE");
            com1_puts(" Mask=0x");
            com1_put_hex_byte(g_led_audit_log[idx].report_byte);
            com1_puts("\r\n");

            audit_render_dynamic_state();

            // Arm cooperative screenshot capture on user event if idle and at least 3 seconds elapsed
            if (!is_running_in_vm()) {
                extern bool atoms_screenshot_is_busy(void);
                extern bool atoms_screenshot_request(uint32_t session_id);
                if (!atoms_screenshot_is_busy() && (uptime_sec - last_shot_sec >= 3)) {
                    last_shot_sec = uptime_sec;
                    atoms_screenshot_request(last_log_count);
                }
            }
        }

        // Periodic telemetry refresh every 100,000 ticks (zero flicker)
        if ((tick - last_screen_tick) > 100000) {
            last_screen_tick = tick;
            audit_render_dynamic_state();
        }

        // Controlled 10-Second Observation Window: stream next diagnostic screenshot if idle
        if (!is_running_in_vm()) {
            extern bool atoms_screenshot_is_busy(void);
            extern bool atoms_screenshot_request(uint32_t session_id);
            if (!atoms_screenshot_is_busy() && (uptime_sec - last_shot_sec >= 10)) {
                last_shot_sec = uptime_sec;
                atoms_screenshot_request(uptime_sec);
            }
        }

        __asm__ volatile("pause");
    }
}
