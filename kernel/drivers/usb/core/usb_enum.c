#include "usb_core.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

USBRealtimeDiagnostics g_usb_diag = {0};

volatile uint64_t g_cfg_hdr_req_sent = 0;
volatile uint64_t g_cfg_hdr_event_rcvd = 0;
volatile uint64_t g_cfg_hdr_bytes_rcvd = 0;
volatile uint64_t g_cfg_full_req_sent = 0;
volatile uint64_t g_cfg_full_event_rcvd = 0;
volatile uint64_t g_cfg_full_bytes_rcvd = 0;

volatile uint32_t g_cfg_last_completion_code = 0;
volatile uint32_t g_cfg_last_transfer_length = 0;
volatile uint32_t g_cfg_last_slot_id = 0;
volatile uint32_t g_cfg_last_ep_id = 0;
volatile uint32_t g_cfg_last_trb_type = 0;
volatile bool g_cfg_doorbell_rung = false;
volatile bool g_cfg_event_arrived = false;
volatile bool g_cfg_timeout_occurred = false;
volatile const char* g_cfg_failing_req_name = "NONE";

// External xHCI calls to manage slots and addresses (since xHCI handles this in hardware instead of SET_ADDRESS)
extern uint8_t xhci_enable_slot(void);
extern bool xhci_address_device(uint8_t slot_id, uint8_t port, uint8_t speed);

void usb_device_connected(uint8_t port, uint8_t speed) {
    extern void usb_forensic_init(void);
    usb_forensic_init();

    display_print("[USB ENUM] Device connected on port ");
    extern void display_print_dec(uint64_t);
    display_print_dec(port);
    display_print(" Speed: ");
    display_print_dec(speed);
    display_print("\n");

    // 1. Enable Slot
    uint8_t slot_id = xhci_enable_slot();
    if (slot_id == 0) {
        display_print("[USB ENUM] Failed to enable slot\n");
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(3, false);
        return;
    }
    g_usb_diag.enable_slot_pass = true;
    
    // 2. Address Device (xHCI handles SET_ADDRESS implicitly in Address Device command)
    if (!xhci_address_device(slot_id, port, speed)) {
        display_print("[USB ENUM] Failed to address device\n");
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(5, false);
        return;
    }
    g_usb_diag.address_device_pass = true;
    
    USBDevice dev;
    memset(&dev, 0, sizeof(dev));
    dev.slot_id = slot_id;
    dev.port = port;
    dev.speed = speed;
    dev.address = slot_id; 

    // 3. GET_DESCRIPTOR(Device) First 8 bytes
    USBDeviceDescriptor dev_desc;
    memset(&dev_desc, 0, sizeof(dev_desc));
    
    bool ok = usb_control_transfer(&dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                                   USB_REQ_GET_DESCRIPTOR, (USB_DESC_DEVICE << 8) | 0, 0,
                                   8, &dev_desc);
                                   
    if (!ok) {
        display_print("[USB ENUM] Failed to get Device Descriptor (first 8 bytes)\n");
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(6, false);
        return;
    }
    
    uint8_t raw_bMaxPacketSize0 = dev_desc.bMaxPacketSize0;
    uint32_t decoded_ep0_mps = raw_bMaxPacketSize0;

    // USB 3.0+ Specification Section 9.6.1 & Table 9-11:
    // For SuperSpeed (4) and SuperSpeedPlus (5), bMaxPacketSize0 is an exponent 2^bMaxPacketSize0.
    // The spec mandates bMaxPacketSize0 = 09H, representing 2^9 = 512 bytes.
    // OpenBSD / NetBSD / FreeBSD reference: mps = (1 << dd->bMaxPacketSize0).
    if (dev.speed == 4 || dev.speed == 5) {
        if (raw_bMaxPacketSize0 == 9) {
            decoded_ep0_mps = 512;
        } else if (raw_bMaxPacketSize0 > 0 && raw_bMaxPacketSize0 <= 16) {
            decoded_ep0_mps = (1 << raw_bMaxPacketSize0);
        } else {
            decoded_ep0_mps = 512; // Standard fallback for SuperSpeed EP0
        }
    } else if (dev.speed == 3) {
        // High-Speed (480 Mbps): fixed 64 bytes
        decoded_ep0_mps = (raw_bMaxPacketSize0 == 64) ? 64 : 64;
    } else if (dev.speed == 2) {
        // Low-Speed (1.5 Mbps): fixed 8 bytes
        decoded_ep0_mps = 8;
    } else if (dev.speed == 1) {
        // Full-Speed (12 Mbps): 8, 16, 32, or 64 bytes
        if (raw_bMaxPacketSize0 == 8 || raw_bMaxPacketSize0 == 16 ||
            raw_bMaxPacketSize0 == 32 || raw_bMaxPacketSize0 == 64) {
            decoded_ep0_mps = raw_bMaxPacketSize0;
        } else {
            decoded_ep0_mps = 64; // Safe default
        }
    } else {
        // Unknown speed fallback
        if (raw_bMaxPacketSize0 == 9) {
            decoded_ep0_mps = 512;
        }
    }

    dev.max_packet_size = decoded_ep0_mps;
    
    // Evaluate Context to update EP0 MaxPacketSize in xHCI controller
    extern bool xhci_evaluate_context(uint8_t slot_id, uint32_t max_packet_size);
    if (dev.max_packet_size > 0) {
        xhci_evaluate_context(slot_id, dev.max_packet_size);
    }

    // Forensic Telemetry as mandated by USB diagnostic protocol
    display_print("[USB_EP0_FORENSIC] speed=");
    display_print_dec(dev.speed);
    display_print(" raw_bMaxPacketSize0=");
    display_print_dec(raw_bMaxPacketSize0);
    display_print(" decoded_ep0_mps=");
    display_print_dec(decoded_ep0_mps);
    display_print(" xhci_ep0_mps=");
    display_print_dec(dev.max_packet_size);
    display_print(" slot=");
    display_print_dec(slot_id);
    display_print(" endpoint=0 request=GET_DESCRIPTOR length=18\n");
    
    // Now get the full 18-byte descriptor
    ok = usb_control_transfer(&dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                                   USB_REQ_GET_DESCRIPTOR, (USB_DESC_DEVICE << 8) | 0, 0,
                                   sizeof(USBDeviceDescriptor), &dev_desc);
                                   
    if (!ok) {
        display_print("[USB ENUM] Failed to get full Device Descriptor\n");
        extern void usb_forensic_mark_stage(int stage, bool success);
        usb_forensic_mark_stage(6, false);
        return;
    }
    g_usb_diag.get_descriptor_pass = true;
    extern void usb_forensic_mark_stage(int stage, bool success);
    usb_forensic_mark_stage(6, true); // USB_STAGE_DEVICE_DESCRIPTOR_RECEIVED
    
    dev.vid = dev_desc.idVendor;
    dev.pid = dev_desc.idProduct;
    
    display_print("[USB ENUM] VID=");
    extern void display_print_hex(uint64_t);
    display_print_hex(dev.vid);
    display_print(" PID=");
    display_print_hex(dev.pid);
    display_print(" MaxPkt=");
    display_print_dec(dev.max_packet_size);
    display_print("\n");
    
    USBDevice* reg_dev = usb_register_device(&dev);
    if (!reg_dev) {
        usb_forensic_mark_stage(7, false);
        return;
    }
    
    // 4. GET_DESCRIPTOR(Configuration) - request 9 bytes header with Linux-standard USB_TIME_SETTLE delay & 3x retry engine
    uint8_t config_hdr[9];
    memset(config_hdr, 0, sizeof(config_hdr));
    
    #include "usb_forensic_phase3.h"

    g_cfg_hdr_req_sent++;
    g_cfg_doorbell_rung = true;
    g_cfg_failing_req_name = "CONFIG_HEADER_9B";
    g_usb_phase3.cfg_buf_virt = (uint64_t)config_hdr;

    display_print("[USB ENUM FORENSIC Stage 7] Slot=");
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);
    display_print_dec(reg_dev->slot_id);
    display_print(" MaxPkt="); display_print_dec(reg_dev->max_packet_size);
    display_print(" req_type="); display_print_hex(USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE);
    display_print(" req="); display_print_hex(USB_REQ_GET_DESCRIPTOR);
    display_print(" wValue="); display_print_hex((USB_DESC_CONFIGURATION << 8) | 0);
    display_print(" wIndex=0 wLength=9\n");

    // USB settle delay (~10ms)
    extern void delay_cycles(uint64_t);
    delay_cycles(100000);

    for (int retry = 0; retry < 3; retry++) {
        memset(config_hdr, 0, sizeof(config_hdr));
        ok = usb_control_transfer(reg_dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                                  USB_REQ_GET_DESCRIPTOR, (USB_DESC_CONFIGURATION << 8) | 0, 0,
                                  9, config_hdr);
        if (ok && config_hdr[0] == 9 && config_hdr[1] == 2) {
            break;
        }
        if (retry < 2) {
            display_print("[USB ENUM] Stage 7 Retry "); display_print_dec(retry + 1); display_print("/3...\n");
            delay_cycles(100000);
        }
    }
                                  
    if (!ok) {
        display_print("[USB ENUM] Failed to get Configuration Descriptor header (9B)\n");
        g_cfg_timeout_occurred = true;
        g_usb_phase3.reject_reason = "HEADER_TRANSFER_FAILED";
        usb_forensic_mark_stage(7, false);
        return;
    }
    g_cfg_hdr_event_rcvd++;
    g_cfg_hdr_bytes_rcvd += 9;
    
    // Action D: Record config_hdr pointer and first 4 bytes before parsing
    g_usb_phase3.cfg_buf_virt = (uint64_t)config_hdr;
    for (int i = 0; i < 4; i++) {
        g_usb_phase3.cfg_buf_dump[i] = config_hdr[i];
    }

    uint8_t bLen = config_hdr[0];
    uint8_t bType = config_hdr[1];
    uint16_t wTotalLength = *(uint16_t*)(config_hdr + 2);
    uint8_t bNumIfaces = config_hdr[4];
    uint8_t bCfgValue = config_hdr[5];

    g_usb_phase3.cfg_bLength = bLen;
    g_usb_phase3.cfg_bDescriptorType = bType;
    g_usb_phase3.cfg_wTotalLength = wTotalLength;
    g_usb_phase3.cfg_bNumInterfaces = bNumIfaces;
    g_usb_phase3.cfg_bConfigurationValue = bCfgValue;

    display_print("[USB ENUM] Config bLength="); display_print_dec(bLen);
    display_print(" bType="); display_print_dec(bType);
    display_print(" wTotalLength="); display_print_dec(wTotalLength);
    display_print(" bNumIfaces="); display_print_dec(bNumIfaces);
    display_print("\n");

    if (bLen != 9) {
        g_usb_phase3.reject_reason = "REJECT: bLength != 9";
        display_print("[USB ENUM] REJECT: bLength != 9\n");
        usb_forensic_mark_stage(7, false);
        return;
    }

    if (bType != 2) {
        g_usb_phase3.reject_reason = "REJECT: bType != 2";
        display_print("[USB ENUM] REJECT: bDescriptorType != 2\n");
        usb_forensic_mark_stage(7, false);
        return;
    }

    if (wTotalLength < 9 || wTotalLength > 2048) {
        g_usb_phase3.reject_reason = "REJECT: wTotalLength invalid";
        display_print("[USB ENUM] REJECT: wTotalLength invalid\n");
        usb_forensic_mark_stage(7, false);
        return;
    }

    g_usb_phase3.reject_reason = "NONE (HEADER OK)";

    // Now get the full configuration descriptor
    uint8_t config_buf[256];
    memset(config_buf, 0, sizeof(config_buf));
    uint16_t fetch_len = wTotalLength;
    if (fetch_len > sizeof(config_buf)) fetch_len = sizeof(config_buf);
    
    g_cfg_full_req_sent++;
    g_cfg_doorbell_rung = true;
    g_cfg_failing_req_name = "CONFIG_FULL_DESC";

    ok = usb_control_transfer(reg_dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN | USB_REQ_REC_DEVICE,
                              USB_REQ_GET_DESCRIPTOR, (USB_DESC_CONFIGURATION << 8) | 0, 0,
                              fetch_len, config_buf);
                              
    if (!ok) {
        display_print("[USB ENUM] Failed to get full Configuration Descriptor\n");
        g_cfg_timeout_occurred = true;
        g_usb_phase3.reject_reason = "REJECT: FULL_DESC_TIMEOUT";
        usb_forensic_mark_stage(7, false);
        return;
    }
    g_cfg_full_event_rcvd++;
    g_cfg_full_bytes_rcvd += fetch_len;
    g_usb_phase3.reject_reason = "NONE (STAGE 7 PASS)";
    usb_forensic_mark_stage(7, true); // USB_STAGE_CONFIG_DESCRIPTOR_RECEIVED
    
    // 5. SET_CONFIGURATION (Config value from config descriptor byte 5)
    uint8_t bConfigurationValue = config_buf[5];
    if (bConfigurationValue == 0) bConfigurationValue = 1;

    display_print("[USB ENUM] Sending SET_CONFIGURATION value=");
    display_print_dec(bConfigurationValue);
    display_print("\n");
    
    usb_forensic_mark_stage(8, true); // USB_STAGE_SET_CONFIGURATION_SENT
    ok = usb_control_transfer(reg_dev, USB_REQ_TYPE_STANDARD | USB_REQ_DIR_OUT | USB_REQ_REC_DEVICE,
                              USB_REQ_SET_CONFIGURATION, bConfigurationValue, 0, 0, NULL);
    if (!ok) {
        display_print("[USB ENUM] SET_CONFIGURATION FAILED\n");
        usb_forensic_mark_stage(9, false);
        return;
    }
    g_usb_diag.set_config_pass = true;
    usb_forensic_mark_stage(9, true); // USB_STAGE_SET_CONFIGURATION_COMPLETED
    display_print("[USB ENUM] SET_CONFIGURATION SUCCESS\n");
    
    // 6. Bind Class Drivers (which will issue Configure Endpoint + SET_PROTOCOL + Interrupt IN)
    extern bool usb_bind_drivers(USBDevice* dev, void* config_desc_buffer, uint16_t total_length);
    if (!usb_bind_drivers(reg_dev, config_buf, fetch_len)) {
        display_print("[USB ENUM] No drivers bound to device.\n");
    }
}
