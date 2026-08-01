#include "../include/xhci_ring.h"
#include "kernel/core/lib/include/string.h"

extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);

void xhci_command_ring_init_bte(xhci_command_ring_t* ring, uint32_t num_trbs) {
    if (!ring) return;
    memset(ring, 0, sizeof(xhci_command_ring_t));
    atoms_spinlock_init(&ring->lock, 1);
    
    if (num_trbs == 0) num_trbs = XHCI_DEFAULT_RING_TRBS;
    ring->size = num_trbs;
    
    uint64_t phys = 0;
    ring->trbs = (xhci_trb_t*)xhci_alloc_dma(num_trbs * sizeof(xhci_trb_t), &phys, "CmdRingBTE");
    ring->phys_base = phys;
    memset(ring->trbs, 0, num_trbs * sizeof(xhci_trb_t));
    
    ring->enqueue = 0;
    ring->cycle = 1;
    
    // Link TRB at end
    xhci_trb_build_link(&ring->trbs[num_trbs - 1], ring->phys_base, 0, true, 0);
}
