#include "kernel/drivers/usb/core/usb_registry.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/input/core/hida.h"
#include "kernel/drivers/keyboard/include/keyboard.h"

// Basic HID Report parsing variables for Mouse
static int32_t s_mouse_x = 0;
static int32_t s_mouse_y = 0;

void usb_hid_report_received(USBDevice* dev, uint8_t* report, uint32_t length, uint8_t protocol) {
    if (protocol == 2) { // Mouse
        if (length >= 3) {
            uint8_t buttons = report[0];
            int dx = (int)(int8_t)report[1];
            int dy = (int)(int8_t)report[2];
            
            // Map HID buttons
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

            hida_push_relative(HIDA_BACKEND_USB, dx, dy, hida_buttons, 0);
        }
    } else if (protocol == 1) { // Keyboard
        if (length >= 8) {
            // Simplified HID keyboard parsing
            uint8_t modifiers = report[0];
            bool shift = (modifiers & 0x22) != 0;
            bool ctrl = (modifiers & 0x11) != 0;
            bool alt = (modifiers & 0x44) != 0;
            
            // Just look at first pressed key in array for now
            uint8_t keycode = report[2];
            if (keycode != 0) {
                KeyboardEvent kevt;
                memset(&kevt, 0, sizeof(kevt));
                kevt.keycode = keycode; // Need mapping to BOS keycodes, but simplified for now
                kevt.pressed = true;
                kevt.shift = shift;
                kevt.ctrl = ctrl;
                kevt.alt = alt;
                
                // Map a few common keys
                if (keycode >= 0x04 && keycode <= 0x1D) kevt.ascii = 'a' + (keycode - 0x04);
                if (keycode >= 0x1E && keycode <= 0x27) kevt.ascii = '1' + (keycode - 0x1E);
                if (keycode == 0x2C) kevt.ascii = ' ';
                
                extern void kernel_input_push_key_event(KeyboardEvent* kevt);
                kernel_input_push_key_event(&kevt);
            }
        }
    }
}

static bool usb_hid_bind(USBDevice* dev, USBInterfaceDescriptor* interface_desc, void* config_desc_buffer, uint16_t total_length) {
    extern void display_print_dec(uint64_t);
    display_print("USB_DIAG_3 = HID interface discovered\n");
    display_print("[USB HID] Binding HID device. Subclass: ");
    display_print_dec(interface_desc->bInterfaceSubClass);
    display_print(" Protocol: ");
    display_print_dec(interface_desc->bInterfaceProtocol);
    display_print("\n");

    if (interface_desc->bInterfaceProtocol == 1) {
        display_print("[USB HID] Detected HID Keyboard\n");
    } else if (interface_desc->bInterfaceProtocol == 2) {
        display_print("USB_DIAG_2 = USB mouse device enumerated\n");
        display_print("[USB HID] Detected HID Mouse\n");
    }

    // Find the Endpoint Descriptor
    uint8_t* ptr = (uint8_t*)interface_desc;
    uint8_t* end = (uint8_t*)config_desc_buffer + total_length;
    
    ptr += interface_desc->bLength; // Skip interface descriptor
    
    uint8_t ep_address = 0;
    uint16_t max_packet_size = 0;
    
    while (ptr < end) {
        USBDescriptorHeader* hdr = (USBDescriptorHeader*)ptr;
        if (hdr->bLength == 0) break;
        
        if (hdr->bDescriptorType == USB_DESC_ENDPOINT) {
            // Endpoint descriptor
            uint8_t bEndpointAddress = *(ptr + 2);
            uint8_t bmAttributes = *(ptr + 3);
            uint16_t wMaxPacketSize = *(uint16_t*)(ptr + 4);
            
            if ((bEndpointAddress & 0x80) && (bmAttributes & 0x03) == 0x03) {
                // Interrupt IN endpoint
                ep_address = bEndpointAddress & 0x0F;
                max_packet_size = wMaxPacketSize;
                break;
            }
        }
        ptr += hdr->bLength;
    }
    
    if (ep_address == 0) {
        display_print("[USB HID] Error: No Interrupt IN endpoint found.\n");
        return false;
    }
    
    // Set Idle to 0 (infinity) to avoid constant reporting if no state changes
    usb_control_transfer(dev, USB_REQ_TYPE_CLASS | USB_REQ_DIR_OUT | USB_REQ_REC_INTERFACE,
                         USB_REQ_SET_IDLE, 0, interface_desc->bInterfaceNumber, 0, NULL);
    
    // Start continuous polling (Interrupt In)
    extern void* pmm_alloc_page(); 
    uint8_t* report_buf = (uint8_t*)pmm_alloc_page();
    
    dev->driver_data = report_buf;
    
    bool started = usb_interrupt_in_transfer(dev, ep_address, max_packet_size, report_buf, max_packet_size);
    if (started) {
        display_print("USB_DIAG_4 = interrupt IN endpoint discovered (EP: ");
        display_print_dec(ep_address);
        display_print(")\n");
        display_print("USB_DIAG_5 = interrupt transfer submitted\n");
        display_print("[USB HID] Starting Interrupt IN transfer on EP ");
        display_print_dec(ep_address);
        display_print("\n");
        return true;
    } else {
        display_print("[USB HID] Error: Failed to start Interrupt IN transfer on EP ");
        display_print_dec(ep_address);
        display_print("\n");
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
    display_print("[USB HID] HID Class Driver initialized\n");
}
