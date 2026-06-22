#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * BOS Rule 18 (Mandatory):
 * The Kernel Heap NEVER owns memory.
 * The Heap ONLY manages allocations.
 * Only the VMM may grow or shrink the heap.
 * The Heap MUST NEVER call the PMM directly.
 */

#define HEAP_MAGIC 0xB05B05

typedef struct HeapBlockHeader {
    uint32_t magic;
    uint32_t size;
    uint32_t flags;
} HeapBlockHeader;

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
