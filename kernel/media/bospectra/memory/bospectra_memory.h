#ifndef BOSPECTRA_MEMORY_H
#define BOSPECTRA_MEMORY_H

#include "../include/bospectra_types.h"

#define BOSPECTRA_DEFAULT_ALIGNMENT 64U

typedef struct {
    size_t   total_allocated_bytes;
    size_t   peak_allocated_bytes;
    uint32_t active_allocation_count;
    uint32_t total_allocations;
    uint32_t total_frees;
} BOSPECTRA_MemoryStats;

void              bospectra_memory_init(void);
void              bospectra_memory_shutdown(void);
void*             bospectra_mem_alloc(size_t size, const char* tag);
void*             bospectra_mem_alloc_aligned(size_t size, size_t alignment, const char* tag);
void              bospectra_mem_free(void* ptr);
void              bospectra_memory_get_stats(BOSPECTRA_MemoryStats* out_stats);
size_t            bospectra_memory_get_live_bytes(void);

#endif // BOSPECTRA_MEMORY_H
