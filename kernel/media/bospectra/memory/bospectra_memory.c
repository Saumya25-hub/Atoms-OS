#include "bospectra_memory.h"
#include "kernel/core/memory/heap/include/heap.h"

static BOSPECTRA_MemoryStats g_bospectra_mem_stats;
static bool g_bospectra_mem_initialized = false;

void bospectra_memory_init(void) {
    g_bospectra_mem_stats.total_allocated_bytes = 0;
    g_bospectra_mem_stats.peak_allocated_bytes = 0;
    g_bospectra_mem_stats.active_allocation_count = 0;
    g_bospectra_mem_stats.total_allocations = 0;
    g_bospectra_mem_stats.total_frees = 0;
    g_bospectra_mem_initialized = true;
}

void bospectra_memory_shutdown(void) {
    g_bospectra_mem_initialized = false;
}

void* bospectra_mem_alloc(size_t size, const char* tag) {
    (void)tag;
    if (size == 0) return NULL;
    void* ptr = kmalloc(size);
    if (ptr) {
        g_bospectra_mem_stats.total_allocated_bytes += size;
        if (g_bospectra_mem_stats.total_allocated_bytes > g_bospectra_mem_stats.peak_allocated_bytes) {
            g_bospectra_mem_stats.peak_allocated_bytes = g_bospectra_mem_stats.total_allocated_bytes;
        }
        g_bospectra_mem_stats.active_allocation_count++;
        g_bospectra_mem_stats.total_allocations++;
    }
    return ptr;
}

void* bospectra_mem_alloc_aligned(size_t size, size_t alignment, const char* tag) {
    (void)tag;
    if (size == 0) return NULL;
    if (alignment == 0) alignment = BOSPECTRA_DEFAULT_ALIGNMENT;
    void* ptr = kmalloc_aligned(size, alignment);
    if (ptr) {
        g_bospectra_mem_stats.total_allocated_bytes += size;
        if (g_bospectra_mem_stats.total_allocated_bytes > g_bospectra_mem_stats.peak_allocated_bytes) {
            g_bospectra_mem_stats.peak_allocated_bytes = g_bospectra_mem_stats.total_allocated_bytes;
        }
        g_bospectra_mem_stats.active_allocation_count++;
        g_bospectra_mem_stats.total_allocations++;
    }
    return ptr;
}

void bospectra_mem_free(void* ptr) {
    if (!ptr) return;
    kfree(ptr);
    if (g_bospectra_mem_stats.active_allocation_count > 0) {
        g_bospectra_mem_stats.active_allocation_count--;
    }
    g_bospectra_mem_stats.total_frees++;
}

void bospectra_memory_get_stats(BOSPECTRA_MemoryStats* out_stats) {
    if (out_stats) {
        *out_stats = g_bospectra_mem_stats;
    }
}

size_t bospectra_memory_get_live_bytes(void) {
    return g_bospectra_mem_stats.total_allocated_bytes;
}
