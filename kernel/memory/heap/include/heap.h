#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * BOS Rule 18 (Mandatory):
 * The Kernel Heap NEVER owns memory.
 * The Heap ONLY manages allocations.
 * Only the VMM may grow or shrink the heap.
 * The Heap MUST NEVER call the PMM directly.
 */

#define HEAP_MAGIC 0x123890AB

typedef struct heap_block {
    uint32_t magic;
    size_t size;          // Size of the usable memory area (excluding this header)
    bool is_free;         // True if this block is free
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

typedef struct HeapStats {
    uint32_t total_size;
    uint32_t used_size;
    uint32_t free_size;
    uint32_t block_count;
    uint32_t largest_free;
} HeapStats;

void heap_init(void);
void* kmalloc(size_t size);
void* kcalloc(size_t num, size_t size);
void* krealloc(void* ptr, size_t new_size);
void kfree(void* ptr);
void heap_get_stats(HeapStats* stats);
void heap_dump_blocks(void);
void heap_stress_test(void);
void heap_validate(void);
void heap_walk(void);
void heap_trace_toggle(void);

#endif // HEAP_H
