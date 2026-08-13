#include "kernel/drivers/usb/core/usb_registry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/input/core/hida.h"
#include "kernel/drivers/keyboard/include/keyboard.h"

#define BOS_KEY_CAPSLOCK 0x9B

// USB HID Usage ID to BOS Keycode Table (Usage IDs 0x00 to 0x53)
static const uint8_t hid_to_bos_keycode[0x54] = {
    /* 0x00 - 0x03 */ 0, 0, 0, 0,
    /* 0x04 - 0x0D */ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    /* 0x0E - 0x17 */ 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    /* 0x18 - 0x1D */ 'u', 'v', 'w', 'x', 'y', 'z',
    /* 0x1E - 0x27 */ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    /* 0x28 */ '\n', /* 0x29 */ 27, /* 0x2A */ '\b', /* 0x2B */ '\t', /* 0x2C */ ' ',
    /* 0x2D - 0x38 */ '-', '=', '[', ']', '\\', 0, ';', '\'', '`', ',', '.', '/',
    /* 0x39 */ BOS_KEY_CAPSLOCK,
    /* 0x3A - 0x45 */ BOS_KEY_F1, BOS_KEY_F2, BOS_KEY_F3, BOS_KEY_F4, BOS_KEY_F5, BOS_KEY_F6,
                      BOS_KEY_F7, BOS_KEY_F8, BOS_KEY_F9, BOS_KEY_F10, BOS_KEY_F11, BOS_KEY_F12,
    /* 0x46 - 0x4E */ 0, 0, 0, BOS_KEY_INS, BOS_KEY_HOME, BOS_KEY_PGUP, BOS_KEY_DEL, BOS_KEY_END, BOS_KEY_PGDN,
    /* 0x4F - 0x52 */ BOS_KEY_RIGHT, BOS_KEY_LEFT, BOS_KEY_DOWN, BOS_KEY_UP,
    /* 0x53 */ BOS_KEY_NUMLOCK
};

static uint8_t prev_kbd_report[8] = {0};
static bool s_caps_lock_state = false;
static bool s_num_lock_state = true;

// Public live telemetry for real-time diagnostic dashboard
volatile bool g_caps_lock_state = false;
volatile bool g_num_lock_state = true;
volatile bool g_scroll_lock_state = false;
volatile uint8_t g_last_key_usage = 0;
volatile uint8_t g_last_key_mapped = 0;
volatile char g_last_key_ascii = 0;
volatile uint8_t g_last_key_modifiers = 0;
volatile uint8_t g_last_kbd_raw_report[8] = {0};
volatile uint32_t g_kbd_total_keypresses = 0;
volatile uint8_t g_kbd_interface_num = 0;
volatile uint8_t g_kbd_ep_addr = 0;

void usb_hid_set_leds(USBDevice* dev, uint8_t leds) {
    uint8_t led_buf = leds;
    uint8_t iface = dev ? dev->interface_number : 0;
    usb_control_transfer(dev, USB_REQ_TYPE_CLASS | USB_REQ_DIR_OUT | USB_REQ_REC_INTERFACE,
                         0x09, (2 << 8) | 0, iface, 1, &led_buf);
}

void usb_hid_report_received(USBDevice* dev, uint8_t* report, uint32_t length, uint8_t protocol) {
    if (!report || length == 0) return;

    extern volatile uint64_t g_usb_hid_packets;
    g_usb_hid_packets++;

    bool is_keyboard = false;
    if (protocol == 1 || (dev && dev->protocol == 1)) {
        is_keyboard = true;
    } else if (dev && dev->protocol == 2) {
        is_keyboard = false;
    } else if (length == 8 && report[1] == 0) {
        is_keyboard = true;
    }

    if (is_keyboard) {
        extern volatile uint64_t g_keyboard_events;
        g_keyboard_events++;
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(19, true); // USB_STAGE_FIRST_KEYBOARD_PACKET
        g_usb_diag.keyboard_packet_count++;
        
        for (int b = 0; b < 8 && b < (int)length; b++) {
            g_last_kbd_raw_report[b] = report[b];
        }

        uint8_t modifiers = report[0];
        bool shift = (modifiers & 0x22) != 0;
        bool ctrl  = (modifiers & 0x11) != 0;
        bool alt   = (modifiers & 0x44) != 0;

        // Process keypresses in 8-byte Boot Protocol report (bytes 2 to 7)
        for (int i = 2; i < 8; i++) {
            uint8_t usage_id = report[i];
            if (usage_id == 0) continue;

            // Check if key is new (not in previous report)
            bool is_new = true;
            for (int k = 2; k < 8; k++) {
                if (prev_kbd_report[k] == usage_id) {
                    is_new = false;
                    break;
                }
            }

            if (is_new) {
                // Check CapsLock toggle (Usage 0x39)
                if (usage_id == 0x39) {
                    s_caps_lock_state = !s_caps_lock_state;
                    g_caps_lock_state = s_caps_lock_state;
                }
                // Check NumLock toggle (Usage 0x53)
                if (usage_id == 0x53) {
                    s_num_lock_state = !s_num_lock_state;
                    g_num_lock_state = s_num_lock_state;
                }
                // Check ScrollLock toggle (Usage 0x47)
                if (usage_id == 0x47) {
                    g_scroll_lock_state = !g_scroll_lock_state;
                }

                KeyboardEvent kevt;
                memset(&kevt, 0, sizeof(kevt));
                kevt.pressed = true;
                kevt.shift = shift;
                kevt.ctrl = ctrl;
                kevt.alt = alt;
                kevt.caps_lock = s_caps_lock_state;

                if (usage_id < sizeof(hid_to_bos_keycode)) {
                    uint8_t mapped = hid_to_bos_keycode[usage_id];
                    kevt.keycode = mapped ? mapped : usage_id;
                    if (mapped >= 'a' && mapped <= 'z') {
                        bool uppercase = shift ^ s_caps_lock_state;
                        kevt.ascii = uppercase ? ('A' + (mapped - 'a')) : mapped;
                    } else if (mapped >= '1' && mapped <= '9') {
                        const char shift_nums[] = "!@#$%^&*()";
                        kevt.ascii = shift ? shift_nums[mapped - '1'] : mapped;
                    } else {
                        kevt.ascii = mapped;
                    }
                } else {
                    kevt.keycode = usage_id;
                }

                g_last_key_usage = usage_id;
                g_last_key_mapped = kevt.keycode;
                g_last_key_ascii = kevt.ascii;
                g_last_key_modifiers = modifiers;
                g_kbd_total_keypresses++;

                extern void hida_push_keyboard_event(uint32_t backend_id, const void* kevt);
                hida_push_keyboard_event(HIDA_BACKEND_USB_KBD, &kevt);
            }
        }

        for (int i = 0; i < 8; i++) {
            prev_kbd_report[i] = report[i];
        }
        return; // Strictly return so keyboard keycodes never enter mouse pipeline!
    }

    // =========================================================================
    // MOUSE REPORT DECODING (Only reached for actual mouse reports)
    // =========================================================================
    extern volatile uint64_t g_mouse_events;
    g_mouse_events++;
    uint8_t buttons = 0;
    int dx = 0;
    int dy = 0;
    int scroll = 0;

    if (length == 3) {
        // Standard 3-byte Boot mouse: [Buttons, dX, dY]
        buttons = report[0];
        dx = (int)(int8_t)report[1];
        dy = (int)(int8_t)report[2];
    } else if (length == 4) {
        if (report[0] <= 0x07) {
            // 4-byte standard mouse with wheel: [Buttons, dX, dY, Wheel]
            buttons = report[0];
            dx = (int)(int8_t)report[1];
            dy = (int)(int8_t)report[2];
            scroll = (int)(int8_t)report[3];
        } else {
            // 4-byte Report ID mouse: [ReportID, Buttons, dX, dY]
            buttons = report[1];
            dx = (int)(int8_t)report[2];
            dy = (int)(int8_t)report[3];
        }
    } else if (length == 5) {
        if (report[0] <= 0x07 && (report[2] == 0 || report[2] == 0xFF || report[4] == 0 || report[4] == 0xFF)) {
            // 5-byte 16-bit displacement mouse (e.g. Gaming/RGB mouse): [Buttons, dX_lo, dX_hi, dY_lo, dY_hi]
            buttons = report[0];
            int16_t raw_x = (int16_t)((uint16_t)report[1] | ((uint16_t)report[2] << 8));
            int16_t raw_y = (int16_t)((uint16_t)report[3] | ((uint16_t)report[4] << 8));
            dx = (int)raw_x;
            dy = (int)raw_y;
        } else if (report[0] >= 1 && report[0] <= 4 && report[1] <= 0x07) {
            // 5-byte Report ID mouse: [ReportID, Buttons, dX, dY, Wheel]
            buttons = report[1];
            dx = (int)(int8_t)report[2];
            dy = (int)(int8_t)report[3];
            scroll = (int)(int8_t)report[4];
        } else {
            buttons = report[0];
            dx = (int)(int8_t)report[1];
            dy = (int)(int8_t)report[2];
            scroll = (int)(int8_t)report[3];
        }
    } else if (length >= 6) {
        if (report[0] <= 0x07) {
            // 6+ byte 16-bit displacement mouse: [Buttons, dX_lo, dX_hi, dY_lo, dY_hi, Wheel, ...]
            buttons = report[0];
            int16_t raw_x = (int16_t)((uint16_t)report[1] | ((uint16_t)report[2] << 8));
            int16_t raw_y = (int16_t)((uint16_t)report[3] | ((uint16_t)report[4] << 8));
            dx = (int)raw_x;
            dy = (int)raw_y;
            if (length >= 6) scroll = (int)(int8_t)report[5];
        } else if (report[0] >= 1 && report[0] <= 4 && report[1] <= 0x07) {
            // Report ID with 16-bit displacement: [ReportID, Buttons, dX_lo, dX_hi, dY_lo, dY_hi, Wheel, ...]
            buttons = report[1];
            int16_t raw_x = (int16_t)((uint16_t)report[2] | ((uint16_t)report[3] << 8));
            int16_t raw_y = (int16_t)((uint16_t)report[4] | ((uint16_t)report[5] << 8));
            dx = (int)raw_x;
            dy = (int)raw_y;
            if (length >= 7) scroll = (int)(int8_t)report[6];
        } else {
            buttons = report[0];
            dx = (int)(int8_t)report[1];
            dy = (int)(int8_t)report[2];
        }
    } else {
        buttons = report[0];
        dx = (int)(int8_t)report[1];
        dy = (int)(int8_t)report[2];
    }
    
    uint8_t hida_buttons = 0;
    if (buttons & 0x01) hida_buttons |= 0x01; // Left
    if (buttons & 0x02) hida_buttons |= 0x02; // Right
    if (buttons & 0x04) hida_buttons |= 0x04; // Middle

    extern volatile uint64_t g_usb_motion_reports_count;
    g_usb_motion_reports_count++;
    
    extern uint64_t timer_get_ticks(void);
    static uint64_t last_hid_ms = 0;
    uint64_t now_ms = timer_get_ticks();
    if (last_hid_ms != 0) {
        extern volatile uint64_t g_hid_max_gap_ms;
        uint64_t gap = now_ms - last_hid_ms;
        if (gap > g_hid_max_gap_ms) g_hid_max_gap_ms = gap;
    }
    last_hid_ms = now_ms;

    if (dx != 0 || dy != 0) {
        extern volatile uint64_t g_hid_decoded_motion_count;
        g_hid_decoded_motion_count++;
    }

    g_usb_diag.mouse_packet_count++;
    extern void usb_forensic_mark_stage(int stage, bool success);
    usb_forensic_mark_stage(18, true); // USB_STAGE_FIRST_MOUSE_PACKET
    hida_push_relative(HIDA_BACKEND_USB, dx, dy, hida_buttons, scroll);
}

static bool usb_hid_bind(USBDevice* dev, USBInterfaceDescriptor* interface_desc, void* config_desc_buffer, uint16_t total_length) {
    extern void display_print_dec(uint64_t);
    display_print("[USB HID] Binding HID device. Subclass: ");
    display_print_dec(interface_desc->bInterfaceSubClass);
    display_print(" Protocol: ");
    display_print_dec(interface_desc->bInterfaceProtocol);
    display_print("\n");

    dev->protocol = interface_desc->bInterfaceProtocol;
    dev->interface_number = interface_desc->bInterfaceNumber;
    if (interface_desc->bInterfaceProtocol == 1) {
        display_print("[USB HID] Detected HID Keyboard\n");
    } else if (interface_desc->bInterfaceProtocol == 2) {
        display_print("[USB HID] Detected HID Mouse\n");
    }

    // 1. Issue SET_PROTOCOL = 0 (Boot Protocol) only if interface supports Boot Subclass
    if (interface_desc->bInterfaceSubClass == 1) {
        usb_control_transfer(dev, USB_REQ_TYPE_CLASS | USB_REQ_DIR_OUT | USB_REQ_REC_INTERFACE,
                             0x0B, 0, interface_desc->bInterfaceNumber, 0, NULL);
        display_print("[USB HID] SET_PROTOCOL (Boot Protocol = 0) Sent\n");
    }

    // 2. Set Idle to 0 (infinity)
    usb_control_transfer(dev, USB_REQ_TYPE_CLASS | USB_REQ_DIR_OUT | USB_REQ_REC_INTERFACE,
                         USB_REQ_SET_IDLE, 0, interface_desc->bInterfaceNumber, 0, NULL);
    display_print("[USB HID] SET_IDLE (0) Sent\n");

    // If keyboard, send initial LED report (NumLock ON)
    if (interface_desc->bInterfaceProtocol == 1) {
        uint8_t init_leds = (s_num_lock_state ? 1 : 0) | (s_caps_lock_state ? 2 : 0);
        usb_hid_set_leds(dev, init_leds);
    }

    // 3. Find the Interrupt IN Endpoint Descriptor
    uint8_t* ptr = (uint8_t*)interface_desc;
    uint8_t* end = (uint8_t*)config_desc_buffer + total_length;
    
    ptr += interface_desc->bLength; // Skip interface descriptor
    
    uint8_t ep_address = 0;
    uint16_t max_packet_size = 0;
    
    while (ptr < end) {
        USBDescriptorHeader* hdr = (USBDescriptorHeader*)ptr;
        if (hdr->bLength == 0) break;
        
        if (hdr->bDescriptorType == USB_DESC_ENDPOINT) {
            uint8_t bEndpointAddress = *(ptr + 2);
            uint8_t bmAttributes = *(ptr + 3);
            uint16_t wMaxPacketSize = *(uint16_t*)(ptr + 4);
            
            if ((bEndpointAddress & 0x80) && (bmAttributes & 0x03) == 0x03) {
                ep_address = bEndpointAddress & 0x0F;
                max_packet_size = wMaxPacketSize;
                if (interface_desc->bInterfaceProtocol == 1) {
                    g_kbd_interface_num = interface_desc->bInterfaceNumber;
                    g_kbd_ep_addr = ep_address;
                }
                display_print("[USB HID] Found Interrupt IN EP ");
                display_print_dec(ep_address);
                display_print(" MaxPkt=");
                display_print_dec(max_packet_size);
                display_print("\n");
                break;
            }
        }
        ptr += hdr->bLength;
    }
    
    if (ep_address == 0) {
        display_print("[USB HID] Error: No Interrupt IN endpoint found.\n");
        return false;
    }
    
    // 4. Configure Endpoint + Start Interrupt IN polling
    extern void* pmm_alloc_page(); 
    uint8_t* report_buf = (uint8_t*)pmm_alloc_page();
    dev->driver_data = report_buf;
    
    bool started = usb_interrupt_in_transfer(dev, ep_address, max_packet_size, report_buf, max_packet_size);
    if (started) {
        g_usb_diag.configure_ep_pass = true;
        g_usb_diag.interrupt_in_pass = true;
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(16, true); // USB_STAGE_HID_BIND_COMPLETED
        display_print("[USB HID] Interrupt IN polling started on EP ");
        display_print_dec(ep_address);
        display_print("\n");
        return true;
    } else {
        display_print("[USB HID] Error: Failed to start Interrupt IN transfer\n");
        return false;
    }
}

void usb_hid_init(void) {
    USBClassDriver hid_driver;
    memset(&hid_driver, 0, sizeof(hid_driver));
    hid_driver.class_code = 0x03; // HID
    hid_driver.subclass_code = 0xFF; // Any
    hid_driver.protocol_code = 0xFF; // Any
    hid_driver.bind = usb_hid_bind;
    hid_driver.name = "USB_HID_Class_Driver";
    
    usb_register_class_driver(hid_driver);
    display_print("[USB HID] Universal Mouse & Keyboard HID Class Driver initialized\n");
}
