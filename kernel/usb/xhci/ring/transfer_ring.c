#include "../include/xhci_ring.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

extern void* xhci_alloc_dma(size_t size, uint64_t* phys_out, const char* name);

void xhci_transfer_ring_init(xhci_transfer_ring_t* ring, uint32_t num_trbs) {
    if (!ring) return;
    memset(ring, 0, sizeof(xhci_transfer_ring_t));
    atoms_spinlock_init(&ring->lock, 1);
    
    if (num_trbs == 0) num_trbs = XHCI_DEFAULT_RING_TRBS;
    ring->size = num_trbs;
    
    uint64_t bytes = num_trbs * sizeof(xhci_trb_t);
    uint64_t phys = 0;
    ring->trbs = (xhci_trb_t*)xhci_alloc_dma(bytes, &phys, "TransferRing");
    ring->phys_base = phys;
    
    memset(ring->trbs, 0, bytes);
    ring->enqueue = 0;
    ring->dequeue = 0;
    ring->cycle = 1; // Cycle Bit starts at 1
    
    // Setup Link TRB at the end of the ring pointing back to index 0
    uint32_t last_idx = num_trbs - 1;
    xhci_trb_build_link(&ring->trbs[last_idx], ring->phys_base, 0, true, 0); // Cycle initially 0 for link TRB
}

void xhci_transfer_ring_free(xhci_transfer_ring_t* ring) {
    if (!ring || !ring->trbs) return;
    // PMM physical page freeing
    pmm_free_pages((void*)ring->phys_base, (ring->size * sizeof(xhci_trb_t) + 4095) / 4096);
    memset(ring, 0, sizeof(xhci_transfer_ring_t));
}

bool xhci_transfer_ring_enqueue_normal(xhci_transfer_ring_t* ring, uint64_t phys_buf, uint32_t len, uint32_t flags, uint32_t* out_index) {
    if (!ring || !ring->trbs) return false;
    
    atoms_irq_lock_state_t lock_state = atoms_spin_lock_irqsave(&ring->lock);
    
    uint32_t curr = ring->enqueue;
    uint32_t next = curr + 1;
    
    // Check if next TRB is Link TRB
    if (next == (ring->size - 1)) {
        // Prepare Link TRB with current cycle state
        xhci_trb_t* link_trb = &ring->trbs[next];
        xhci_trb_build_link(link_trb, ring->phys_base, 0, true, ring->cycle);
        
        // Wrap enqueue index back to 0 and toggle cycle bit
        ring->enqueue = 0;
        ring->cycle ^= 1;
        ring->ring_wraps++;
    } else {
        ring->enqueue = next;
    }
    
    // Write Normal TRB
    xhci_trb_t* trb = &ring->trbs[curr];
    xhci_trb_build_normal(trb, phys_buf, len, 0, flags, ring->cycle);
    
    asm volatile ("mfence" ::: "memory");
    
    if (out_index) *out_index = curr;
    ring->total_enqueued++;
    
    atoms_spin_unlock_irqrestore(&ring->lock, lock_state);
    return true;
}
