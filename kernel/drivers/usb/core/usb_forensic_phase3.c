#include "usb_forensic_phase3.h"

USBPhase3Forensic g_usb_phase3 = {0};

void usb_phase3_init(void) {
    g_usb_phase3.current_request[0] = 'I';
    g_usb_phase3.current_request[1] = 'N';
    g_usb_phase3.current_request[2] = 'I';
    g_usb_phase3.current_request[3] = 'T';
    g_usb_phase3.current_request[4] = '\0';

    g_usb_phase3.bmRequestType = 0;
    g_usb_phase3.bRequest = 0;
    g_usb_phase3.wValue = 0;
    g_usb_phase3.wIndex = 0;
    g_usb_phase3.wLength = 0;

    g_usb_phase3.cfg_bLength = 0;
    g_usb_phase3.cfg_bDescriptorType = 0;
    g_usb_phase3.cfg_wTotalLength = 0;
    g_usb_phase3.cfg_bNumInterfaces = 0;
    g_usb_phase3.cfg_bConfigurationValue = 0;
    g_usb_phase3.reject_reason = "NONE";

    g_usb_phase3.dma_phys = 0;
    g_usb_phase3.dma_virt = 0;
    g_usb_phase3.user_buf_virt = 0;
    g_usb_phase3.cfg_buf_virt = 0;
    g_usb_phase3.dma_forensic_result = "INITIALIZING";

    for (int i = 0; i < 32; i++) g_usb_phase3.dma_dump[i] = 0;
    for (int i = 0; i < 4; i++) g_usb_phase3.cfg_buf_dump[i] = 0;

    g_usb_phase3.setup_dw0 = 0; g_usb_phase3.setup_dw1 = 0; g_usb_phase3.setup_dw2 = 0; g_usb_phase3.setup_dw3 = 0;
    g_usb_phase3.data_dw0 = 0;  g_usb_phase3.data_dw1 = 0;  g_usb_phase3.data_dw2 = 0;  g_usb_phase3.data_dw3 = 0;
    g_usb_phase3.status_dw0 = 0; g_usb_phase3.status_dw1 = 0; g_usb_phase3.status_dw2 = 0; g_usb_phase3.status_dw3 = 0;

    g_usb_phase3.doorbell_slot = 0;
    g_usb_phase3.doorbell_target = 0;

    g_usb_phase3.ep0_enq_before = 0;
    g_usb_phase3.ep0_enq_after = 0;
    g_usb_phase3.ep0_cyc_before = 0;
    g_usb_phase3.ep0_cyc_after = 0;

    g_usb_phase3.evt_deq = 0;
    g_usb_phase3.evt_cyc = 0;

    g_usb_phase3.setup_trb_sent = false;
    g_usb_phase3.data_trb_sent = false;
    g_usb_phase3.status_trb_sent = false;
    g_usb_phase3.doorbell_rung = false;
    g_usb_phase3.transfer_event_received = false;
    g_usb_phase3.completion_code = 0;
    g_usb_phase3.residual_length = 0;
    g_usb_phase3.timeout_occurred = false;
}
