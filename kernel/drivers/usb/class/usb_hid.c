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
            int8_t dx = (int8_t)report[1];
            int8_t dy = (int8_t)report[2];
            
            // Map HID buttons
            uint8_t hida_buttons = 0;
            if (buttons & 0x01) hida_buttons |= 0x01; // Left
            if (buttons & 0x02) hida_buttons |= 0x02; // Right
            if (buttons & 0x04) hida_buttons |= 0x04; // Middle

            static uint32_t s_report_count = 0;
            if (s_report_count < 10) {
                display_print("[USB HID] Mouse Report #");
                extern void display_print_dec(uint64_t);
                display_print_dec(s_report_count + 1);
                display_print(": DX="); display_print_dec((uint8_t)dx);
                display_print(" DY="); display_print_dec((uint8_t)dy);
                display_print(" BTN="); display_print_dec(buttons);
                display_print("\n");
                s_report_count++;
            }
            
            s_mouse_x += dx;
            s_mouse_y += dy; // Some mice might need -dy
            
            // Clamp to screen bounds (assuming 1024x768 for now, but CCTE handles actual bounds)
            if (s_mouse_x < 0) s_mouse_x = 0;
            if (s_mouse_y < 0) s_mouse_y = 0;
            if (s_mouse_x > 1023) s_mouse_x = 1023;
            if (s_mouse_y > 767) s_mouse_y = 767;
            
            hida_push_absolute(HIDA_BACKEND_USB, s_mouse_x, s_mouse_y, 1024, 768, hida_buttons, 0);
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
    display_print("[USB HID] Binding HID device. Subclass: ");
    extern void display_print_dec(uint64_t);
    display_print_dec(interface_desc->bInterfaceSubClass);
    display_print(" Protocol: ");
    display_print_dec(interface_desc->bInterfaceProtocol);
    display_print("\n");

    if (interface_desc->bInterfaceProtocol == 1) {
        display_print("[USB HID] Detected HID Keyboard\n");
    } else if (interface_desc->bInterfaceProtocol == 2) {
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
    
    display_print("[USB HID] Starting Interrupt IN transfer on EP ");
    display_print_dec(ep_address);
    display_print("\n");
    
    usb_interrupt_in_transfer(dev, ep_address, max_packet_size, report_buf, max_packet_size);
    
    return true;
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
