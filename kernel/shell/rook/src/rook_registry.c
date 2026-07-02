#include "kernel/shell/rook/include/rook.h"
#include "kernel/core/lib/include/string.h"

/*
 * ♜ ROOK ENGINE V1.0 — Static O(1) Page Registry
 * Chess Rook Philosophy: Permanent numeric index addressing.
 * No linked list traversal, no string routing, zero dynamic allocations.
 */

static rook_page_t* rook_page_table[ROOK_MAX_PAGES];

int rook_register_page(rook_page_t* page) {
    if (!page) return -1;
    if (page->id >= ROOK_MAX_PAGES) return -2;

    rook_page_table[page->id] = page;
    
    /* Execute Create and Initialize lifecycle stages if unallocated */
    if (page->state == ROOK_STATE_UNALLOCATED) {
        if (page->ops.on_create) page->ops.on_create(page);
        page->state = ROOK_STATE_CREATED;
        
        if (page->ops.on_init) page->ops.on_init(page);
        page->state = ROOK_STATE_INITIALIZED;
    }

    return 0;
}

rook_page_t* rook_get_page(uint16_t page_id) {
    if (page_id >= ROOK_MAX_PAGES) return 0;
    return rook_page_table[page_id];
}
