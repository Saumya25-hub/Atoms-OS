#ifndef ATOMS_USB_FORENSIC_PHASE3_H
#define ATOMS_USB_FORENSIC_PHASE3_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char current_request[32];
    
    // Request setup bytes
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;

    // Configuration Descriptor Header Parsed Fields
    uint8_t  cfg_bLength;
    uint8_t  cfg_bDescriptorType;
    uint16_t cfg_wTotalLength;
    uint8_t  cfg_bNumInterfaces;
    uint8_t  cfg_bConfigurationValue;
    const char* reject_reason;
    
    // Phase-4 DMA & Memory Telemetry Dumps
    uint64_t dma_phys;
    uint64_t dma_virt;
    uint64_t user_buf_virt;
    uint8_t  dma_dump[32];
    uint8_t  cfg_buf_dump[4];
    uint64_t cfg_buf_virt;
    const char* dma_forensic_result;

    // TRB Dumps
    uint32_t setup_dw0;
    uint32_t setup_dw1;
    uint32_t setup_dw2;
    uint32_t setup_dw3;
    
    uint32_t data_dw0;
    uint32_t data_dw1;
    uint32_t data_dw2;
    uint32_t data_dw3;
    
    uint32_t status_dw0;
    uint32_t status_dw1;
    uint32_t status_dw2;
    uint32_t status_dw3;
    
    uint32_t doorbell_slot;
    uint32_t doorbell_target;
    
    uint32_t ep0_enq_before;
    uint32_t ep0_enq_after;
    uint8_t  ep0_cyc_before;
    uint8_t  ep0_cyc_after;
    
    uint32_t evt_deq;
    uint8_t  evt_cyc;
    
    // 9 Live Telemetry Panel Flags & Status
    bool setup_trb_sent;
    bool data_trb_sent;
    bool status_trb_sent;
    bool doorbell_rung;
    bool transfer_event_received;
    uint32_t completion_code;
    uint32_t residual_length;
    bool timeout_occurred;
} USBPhase3Forensic;

extern USBPhase3Forensic g_usb_phase3;

void usb_phase3_init(void);

#endif // ATOMS_USB_FORENSIC_PHASE3_H
