#include "../include/xhci_ring.h"
#include "../include/xhci_events.h"
#include "kernel/core/lib/include/string.h"

extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);

void xhci_event_ring_init_bte(xhci_event_ring_t* ring, uint32_t num_trbs) {
    if (!ring) return;
    memset(ring, 0, sizeof(xhci_event_ring_t));
    atoms_spinlock_init(&ring->lock, 1);
    
    if (num_trbs == 0) num_trbs = XHCI_DEFAULT_RING_TRBS;
    ring->size = num_trbs;
    
    uint64_t phys = 0;
    ring->trbs = (xhci_trb_t*)xhci_alloc_dma(num_trbs * sizeof(xhci_trb_t), &phys, "EventRingBTE");
    ring->phys_base = phys;
    memset(ring->trbs, 0, num_trbs * sizeof(xhci_trb_t));
    
    ring->dequeue = 0;
    ring->cycle = 1;
}

bool xhci_event_decode(const xhci_trb_t* event_trb, xhci_event_decoded_t* out_evt) {
    if (!event_trb || !out_evt) return false;
    
    out_evt->trb_pointer = (uint64_t)event_trb->parameter_lo | ((uint64_t)event_trb->parameter_hi << 32);
    out_evt->transfer_length = event_trb->status & 0xFFFFFF;
    out_evt->completion_code = (event_trb->status >> 24) & 0xFF;
    out_evt->trb_type = (event_trb->control >> 10) & 0x3F;
    out_evt->endpoint_id = (event_trb->control >> 16) & 0x1F;
    out_evt->slot_id = (event_trb->control >> 24) & 0xFF;
    
    return true;
}
