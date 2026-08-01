#ifndef SIGNATURES_XHCI_EVENTS_H
#define SIGNATURES_XHCI_EVENTS_H

#include "xhci_trb.h"

typedef struct {
    uint64_t trb_pointer;
    uint32_t transfer_length;
    uint8_t  completion_code;
    uint8_t  slot_id;
    uint8_t  endpoint_id; // DCI (1..31)
    uint8_t  trb_type;
} xhci_event_decoded_t;

// API Declarations
bool xhci_event_decode(const xhci_trb_t* event_trb, xhci_event_decoded_t* out_evt);
void xhci_event_process_transfer(const xhci_event_decoded_t* evt);
void xhci_event_process_command(const xhci_event_decoded_t* evt);

#endif // SIGNATURES_XHCI_EVENTS_H
