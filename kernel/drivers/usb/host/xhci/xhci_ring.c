#include "xhci.h"
#include "kernel/core/lib/include/string.h"

// Initializes an xHCI ring with a given number of TRBs.
// Allocates the memory and sets the initial cycle bit to 1.
void xhci_ring_init(XHCIRing* ring, uint32_t num_trbs) {
    uint64_t phys_base;
    ring->size = num_trbs;
    ring->trbs = (XHCITrb*)xhci_alloc_dma(num_trbs * sizeof(XHCITrb), &phys_base, "Ring");
    ring->phys_base = phys_base;
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->cycle = 1;

    // Set the last TRB as a Link TRB back to the beginning
    XHCITrb* link_trb = &ring->trbs[num_trbs - 1];
    link_trb->param1 = (uint32_t)(phys_base & 0xFFFFFFFF);
    link_trb->param2 = (uint32_t)((phys_base >> 32) & 0xFFFFFFFF);
    link_trb->status = 0;
    // Type is LINK (6), Toggle Cycle (TC) bit is set (bit 1)
    link_trb->control = (TRB_LINK << 10) | (1 << 1); 
}

// Enqueues a TRB to the ring. 
// Note: This does not ring the doorbell.
void xhci_ring_enqueue(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control) {
    XHCITrb* trb = &ring->trbs[ring->enqueue];

    trb->param1 = param1;
    trb->param2 = param2;
    trb->status = status;
    
    // Set the cycle bit to match the current ring cycle state
    control = (control & ~1) | ring->cycle;
    trb->control = control;

    ring->enqueue++;

    // Check if we hit the Link TRB
    if (ring->enqueue == ring->size - 1) {
        XHCITrb* link_trb = &ring->trbs[ring->enqueue];
        // Toggle the link TRB's cycle bit so the controller will evaluate it
        link_trb->control = (link_trb->control & ~1) | ring->cycle;
        
        // Wrap around
        ring->enqueue = 0;
        ring->cycle ^= 1; // Toggle cycle state
    }
}
