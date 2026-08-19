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
    
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);
    display_print("[XHCI RING AUDIT]\n");
    display_print("AllocatedBytes="); display_print_dec(num_trbs * sizeof(XHCITrb));
    display_print(" TRBCount="); display_print_dec(num_trbs);
    display_print(" LinkTRBIndex="); display_print_dec(num_trbs - 1);
    display_print(" RingPhys="); display_print_hex(phys_base); display_print("\n");
}

// Enqueues a TRB to the ring. 
// Note: This does not ring the doorbell.
void xhci_ring_enqueue_raw(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control) {
    /* Guard: ring must be initialized */
    if (!ring || !ring->trbs || ring->size == 0) return;

    XHCITrb* trb = &ring->trbs[ring->enqueue];

    trb->param1 = param1;
    trb->param2 = param2;
    trb->status = status;
    trb->control = control; // Write exact control including cycle bit

    // Invalidate CPU cache line for this TRB so xHCI DMA controller immediately sees it in DRAM
    asm volatile ("clflush (%0)" :: "r"(trb) : "memory");

    ring->enqueue++;

    if (ring->enqueue == ring->size - 1) {
        XHCITrb* link_trb = &ring->trbs[ring->enqueue];
        link_trb->control = (link_trb->control & ~1) | ring->cycle;
        asm volatile ("clflush (%0)" :: "r"(link_trb) : "memory");
        ring->enqueue = 0;
        ring->cycle ^= 1;
        // Clear cycle bit of slot 0 for upcoming lap
        ring->trbs[0].control = (ring->trbs[0].control & ~1) | (ring->cycle ^ 1);
        asm volatile ("clflush (%0)" :: "r"(&ring->trbs[0]) : "memory");
    } else {
        // Clear cycle bit of next upcoming TRB so hardware controller never prematurely pre-fetches stale lap data
        ring->trbs[ring->enqueue].control = (ring->trbs[ring->enqueue].control & ~1) | (ring->cycle ^ 1);
        asm volatile ("clflush (%0)" :: "r"(&ring->trbs[ring->enqueue]) : "memory");
    }
    asm volatile ("mfence" ::: "memory");
}

void xhci_ring_enqueue(XHCIRing* ring, uint32_t param1, uint32_t param2, uint32_t status, uint32_t control) {
    xhci_ring_enqueue_raw(ring, param1, param2, status, (control & ~1) | ring->cycle);
}
