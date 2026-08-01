#include "../include/xhci_debug.h"
#include "kernel/drivers/display/display.h"

void xhci_dump_transfer(const xhci_bulk_request_t* req) {
    if (!req) return;
    display_print("=== XHCI BULK REQUEST DUMP ===\n");
    display_print("Req ID     : "); display_print_dec(req->request_id); display_print("\n");
    display_print("Slot ID    : "); display_print_dec(req->slot_id); display_print("\n");
    display_print("EP Num     : "); display_print_dec(req->ep_num); display_print(req->dir_in ? " (IN)\n" : " (OUT)\n");
    display_print("Virt Buf   : "); display_print_hex((uint64_t)req->virt_buffer); display_print("\n");
    display_print("Phys Buf   : "); display_print_hex(req->phys_buffer); display_print("\n");
    display_print("Trans Len  : "); display_print_dec(req->transfer_len); display_print("\n");
    display_print("Actual Len : "); display_print_dec(req->actual_len); display_print("\n");
    display_print("State      : "); display_print_dec((uint32_t)req->state); display_print("\n");
    display_print("Comp Code  : "); display_print_dec(req->completion_code); display_print("\n");
    display_print("==============================\n");
}

void xhci_dump_ring(const xhci_transfer_ring_t* ring, const char* ring_name) {
    if (!ring) return;
    display_print("--- RING DUMP: "); display_print(ring_name ? ring_name : "UNKNOWN"); display_print(" ---\n");
    display_print("Phys Base : "); display_print_hex(ring->phys_base); display_print("\n");
    display_print("Size      : "); display_print_dec(ring->size); display_print(" TRBs\n");
    display_print("Enqueue   : "); display_print_dec(ring->enqueue); display_print("\n");
    display_print("Dequeue   : "); display_print_dec(ring->dequeue); display_print("\n");
    display_print("Cycle Bit : "); display_print_dec(ring->cycle); display_print("\n");
    display_print("Wraps     : "); display_print_dec(ring->ring_wraps); display_print("\n");
    display_print("-------------------------------\n");
}

void xhci_dump_queue(const xhci_request_queue_t* queue, const char* queue_name) {
    if (!queue) return;
    display_print("[QUEUE DUMP] "); display_print(queue_name ? queue_name : "Queue");
    display_print(" Count = "); display_print_dec(queue->count); display_print("\n");
}
