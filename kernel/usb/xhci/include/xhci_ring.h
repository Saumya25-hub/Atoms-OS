#ifndef SIGNATURES_XHCI_RING_H
#define SIGNATURES_XHCI_RING_H

#include "xhci_trb.h"
#include "kernel/core/sync/spinlock.h"

#define XHCI_DEFAULT_RING_TRBS 256
#define XHCI_MAX_EP_RINGS      64

typedef struct {
    xhci_trb_t* trbs;
    uint64_t    phys_base;
    uint32_t    size;      // Number of TRBs in ring (e.g., 256)
    uint32_t    enqueue;   // Next TRB index to write
    uint32_t    dequeue;   // Next TRB index to consume
    uint8_t     cycle;     // Current Cycle Bit (1 or 0)
    atoms_spinlock_t lock; // IRQ-safe ring lock
    uint64_t    total_enqueued;
    uint64_t    total_dequeued;
    uint64_t    ring_wraps;
} xhci_transfer_ring_t;

typedef struct {
    xhci_trb_t* trbs;
    uint64_t    phys_base;
    uint32_t    size;
    uint32_t    enqueue;
    uint8_t     cycle;
    atoms_spinlock_t lock;
} xhci_command_ring_t;

typedef struct {
    xhci_trb_t* trbs;
    uint64_t    phys_base;
    uint32_t    size;
    uint32_t    dequeue;
    uint8_t     cycle;
    atoms_spinlock_t lock;
} xhci_event_ring_t;

// API Declarations
void xhci_transfer_ring_init(xhci_transfer_ring_t* ring, uint32_t num_trbs);
void xhci_transfer_ring_free(xhci_transfer_ring_t* ring);
bool xhci_transfer_ring_enqueue_normal(xhci_transfer_ring_t* ring, uint64_t phys_buf, uint32_t len, uint32_t flags, uint32_t* out_index);
void xhci_command_ring_init_bte(xhci_command_ring_t* ring, uint32_t num_trbs);
void xhci_event_ring_init_bte(xhci_event_ring_t* ring, uint32_t num_trbs);

#endif // SIGNATURES_XHCI_RING_H
